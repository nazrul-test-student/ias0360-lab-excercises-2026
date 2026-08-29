#!/usr/bin/env bash
set -euo pipefail

ENV_FILE="${HOME}/.env"
SSH_DIR="${HOME}/.ssh"
# The private key itself is kept off the bind-mounted home directory, in the
# container's own filesystem — OpenSSH hard-fails unless the key file is
# exactly 600, and some host filesystems the repo might be checked out onto
# (e.g. an NTFS drive mounted via ntfs-3g) silently ignore chmod, always
# reporting the same fixed permissions no matter what's requested. Keeping
# the key here instead sidesteps that entirely, and as a side effect it
# never touches the student's disk at all.
KEY_DIR="/opt/ssh-keys"
KEY_FILE="${KEY_DIR}/id_ed25519"
# OpenSSH resolves `~/.ssh/config` and `~/.ssh/known_hosts` from the passwd
# database entry for the running UID, not from $HOME — deliberate OpenSSH
# hardening, unrelated to git (which does honor $HOME correctly). Ubuntu's
# base image ships a built-in "ubuntu" user at UID 1000, which collides
# with the near-universal first-user UID on a single-user Linux install, so
# for most students ssh would silently go looking in /home/ubuntu/.ssh
# instead of here. Every path is therefore passed to ssh explicitly via
# git's core.sshCommand below rather than left for ssh to guess.
SSH_CMD="ssh -i ${KEY_FILE} -o IdentitiesOnly=yes -o UserKnownHostsFile=${SSH_DIR}/known_hosts -o StrictHostKeyChecking=accept-new"
SUBMISSION_REPO_URLS=(
    "git@github.com:taltech-eailab-courses/ias0360-home-assignment-1-submission-2026.git"
    "git@github.com:taltech-eailab-courses/ias0360-home-assignment-2-submission-2026.git"
    "git@github.com:taltech-eailab-courses/ias0360-home-assignment-3-submission-2026.git"
)
SUBMISSION_DIR="${HOME}/submission"

# /root (and everything cloned under it) is a bind mount from the host, so
# its UID won't match the container's root user — Git's ownership check
# would otherwise refuse to touch any repo in here.
git config --global --add safe.directory '*' || true

setup_git_ssh() {
    if [[ ! -f "${ENV_FILE}" ]]; then
        echo "No ${ENV_FILE} found — skipping git/SSH setup (copy .env.example to .env to enable it)."
        return 0
    fi

    # shellcheck disable=SC1090
    source "${ENV_FILE}"

    [[ -n "${GIT_USER_NAME:-}" ]] && git config --global user.name "${GIT_USER_NAME}"
    [[ -n "${GIT_USER_EMAIL:-}" ]] && git config --global user.email "${GIT_USER_EMAIL}"

    if [[ -n "${SSH_PRIVATE_KEY:-}" ]]; then
        # Containers are never reused (see build_in_docker.sh), so this
        # directory is always empty at this point — no need to check for an
        # existing key, just (re)install fresh from the current .env every
        # time.
        mkdir -p "${KEY_DIR}"
        chmod 700 "${KEY_DIR}"
        printf '%s\n' "${SSH_PRIVATE_KEY}" > "${KEY_FILE}"
        chmod 600 "${KEY_FILE}"
        echo "Installed SSH key at ${KEY_FILE}"

        mkdir -p "${SSH_DIR}"
        chmod 700 "${SSH_DIR}" 2>/dev/null || true

        touch "${SSH_DIR}/known_hosts"
        if ! grep -q "^github.com " "${SSH_DIR}/known_hosts" 2>/dev/null; then
            ssh-keyscan -H github.com >>"${SSH_DIR}/known_hosts" 2>/dev/null || true
        fi

        git config --global core.sshCommand "${SSH_CMD}"
    fi

    echo "Git identity: ${GIT_USER_NAME:-<unset>} <${GIT_USER_EMAIL:-<unset>}>"
}

clone_submission_repos() {
    if [[ ! -f "${KEY_FILE}" ]]; then
        echo "No SSH key configured — skipping submission repo clones (set SSH_PRIVATE_KEY in .env to enable it)."
        return 0
    fi

    mkdir -p "${SUBMISSION_DIR}"

    local repo_url repo_dir
    for repo_url in "${SUBMISSION_REPO_URLS[@]}"; do
        repo_dir="${SUBMISSION_DIR}/$(basename "${repo_url}" .git)"

        if [[ -d "${repo_dir}" ]]; then
            echo "Submission repo already present at ${repo_dir}, skipping clone."
            continue
        fi

        echo "Cloning submission repo into ${repo_dir}..."
        if ! GIT_SSH_COMMAND="${SSH_CMD} -o BatchMode=yes" git clone "${repo_url}" "${repo_dir}"; then
            echo "Warning: failed to clone ${repo_url}, continuing with the rest."
        fi
    done
}

# The container starts as root so it can do the one thing that genuinely
# needs it: opening up the bind-mounted USB device nodes (owned by root on
# the host) so the *unprivileged* shell below can still reach the Pico.
# Everything else — git config, the SSH key, submission clones, and the
# interactive shell itself — then runs as the student's own host UID/GID,
# via a one-time re-exec of this same script under setpriv, so every file
# created ends up owned by them on the host instead of by root (which they
# can't delete/edit outside the container on a normal, non-sudo lab
# account).
if [[ -z "${ENTRYPOINT_DROPPED:-}" && -n "${HOST_UID:-}" && -n "${HOST_GID:-}" && "${HOST_UID}" != "0" ]]; then
    if [[ -d /dev/bus/usb ]]; then
        chmod -R o+rw /dev/bus/usb 2>/dev/null || true
    fi

    # KEY_DIR, and Jupyter's config/data/runtime dirs, all live in the
    # image's own /opt, root-owned (mode 755) by the Dockerfile's build-time
    # `mkdir`. Hand them to the student's UID now, while still root, so the
    # unprivileged process below (including `jupyter lab`, run interactively
    # from the shell, not by this script) can write into them.
    mkdir -p "${KEY_DIR}"
    chown "${HOST_UID}:${HOST_GID}" "${KEY_DIR}"
    chmod 700 "${KEY_DIR}"
    chown "${HOST_UID}:${HOST_GID}" "${JUPYTER_CONFIG_DIR}" "${JUPYTER_DATA_DIR}" "${JUPYTER_RUNTIME_DIR}" 2>/dev/null || true

    export ENTRYPOINT_DROPPED=1
    exec setpriv --reuid="${HOST_UID}" --regid="${HOST_GID}" --clear-groups --no-new-privs "$0" "$@"
fi

setup_git_ssh || echo "Warning: git/SSH setup failed, continuing without it."
clone_submission_repos || echo "Warning: submission repo clones failed, continuing without them."

exec "$@"
