#!/usr/bin/env bash
set -euo pipefail

IMAGE_NAME="ias0360-2026"
CONTAINER_NAME="ias0360-2026"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_CONTEXT="${SCRIPT_DIR}/own_pc_setup"
CONTAINER_HOME="/root"

usage() {
    echo "Usage: $0 [rebuild|stop]"
    echo "  (no args)  build the image if missing, then start/attach the container"
    echo "  rebuild    stop+remove existing containers/image, rebuild, then attach"
    echo "  stop       stop the running container, if any, and exit"
    exit 1
}

build_image() {
    echo "Building Docker image '${IMAGE_NAME}'..."
    docker build -t "${IMAGE_NAME}" "${DOCKER_CONTEXT}"
}

ARG="${1:-}"
if [[ -n "${ARG}" && "${ARG}" != "rebuild" && "${ARG}" != "stop" ]]; then
    usage
fi

if [[ "${ARG}" == "stop" ]]; then
    if docker ps --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
        echo "Stopping container '${CONTAINER_NAME}'..."
        docker stop "${CONTAINER_NAME}" >/dev/null
    else
        echo "Container '${CONTAINER_NAME}' is not running."
    fi
    exit 0
fi

if [[ "${ARG}" == "rebuild" ]]; then
    echo "Rebuild requested: removing existing containers and image..."

    mapfile -t existing_containers < <(docker ps -aq --filter "ancestor=${IMAGE_NAME}")
    if [[ ${#existing_containers[@]} -gt 0 ]]; then
        docker rm -f "${existing_containers[@]}" >/dev/null
    fi
    if docker ps -a --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
        docker rm -f "${CONTAINER_NAME}" >/dev/null
    fi
    if docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1; then
        docker rmi -f "${IMAGE_NAME}" >/dev/null
    fi

    build_image
fi

if ! docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1; then
    build_image
fi

if docker ps --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
    echo "Container '${CONTAINER_NAME}' is already running, attaching..."
    exec docker exec -it "${CONTAINER_NAME}" bash
fi

if docker ps -a --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
    echo "Starting existing container '${CONTAINER_NAME}'..."
    docker start "${CONTAINER_NAME}" >/dev/null
    exec docker exec -it "${CONTAINER_NAME}" bash
fi

echo "Creating and starting container '${CONTAINER_NAME}'..."
exec docker run -it \
    --name "${CONTAINER_NAME}" \
    --privileged \
    --network host \
    -v "${SCRIPT_DIR}:${CONTAINER_HOME}" \
    -v /dev/bus/usb:/dev/bus/usb \
    -w "${CONTAINER_HOME}" \
    "${IMAGE_NAME}" \
    bash
