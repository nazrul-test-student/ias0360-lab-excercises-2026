import numpy as np

def scale(X, x_min, x_max):
    nom = (X-X.min(axis=0))*(x_max-x_min)
    denom = X.max(axis=0) - X.min(axis=0)
    denom[denom==0] = 1
    return x_min + nom/denom

def data_normalize(raw_data):
    """
    Receive raw training data and returns normalized data and array of maximum value for each column.
    Args:
        raw_data: raw training data

    Returns:
        train_x: normalized data
        max_values: array that contains maximum value for each column
    """
    # TODO 1: implement this method.
    # Hint: use the scale() helper above to scale raw_data into [0, 1],
    # and raw_data.max(axis=0) for max_values.
    norm_data = None
    max_values = None
    return norm_data, max_values

def data_normalize_prediction(raw_data, max_values):
    norm_data=(raw_data - raw_data.min(axis=0))/(raw_data.max(axis=0)- raw_data.min(axis=0))
    return norm_data

def sigmoid(Z):
    return 1 / (1 + np.exp(-Z))

def relu(Z):
    # TODO 2: implement relu function.
    return None

def single_layer_forward_propagation(A_prev, W_curr, b_curr, activation="relu"):
    """Perform single layer forward propagation.

    Args:
        A_prev (np.ndarray): an input vector in previous layer
        W_curr (np.ndarray): a weight vector for the current layer
        b_curr (np.ndarray): a bias vector for the current layer
        activation (str, optional): to specify either relu or sigmoid activation function

    Returns:
        A_curr: calculated activation A matrix
        Z_curr: intermediate Z matrix
    """
    # TODO 3: implement this function.
    # calculation of the input value for the activation function
    Z_curr = None

    # selection of activation function
    if activation == "relu":
        activation_func = relu
    elif activation == "sigmoid":
        activation_func = sigmoid
    else:
        raise Exception('Non-supported activation function')

    # return of calculated activation A and the intermediate Z matrix
    A_curr = None
    return A_curr, Z_curr

def full_forward_propagation(X, params_values):
    """This function performs full forward propagation through all 3 layers
    of the network (5 -> 8 -> 4 -> 1) using the given input vector X and
    params_values that stores the weights and biases.

    Args:
        X (np.ndarray): input vector X, shape (n_features, m_examples)
        params_values (dict): weight and bias vectors stored in a dictionary

    Returns:
        A3: output of the network
        memory: dict of all intermediate A/Z matrices, needed for backprop
    """
    # TODO 4: implement this method.
    # Call single_layer_forward_propagation() three times (relu, relu, sigmoid)
    # and store every intermediate A/Z so backprop can use them later.

    A1, Z1 = None, None
    A2, Z2 = None, None
    A3, Z3 = None, None

    memory = {"A1": A1, "Z1": Z1, "A2": A2, "Z2": Z2, "A3": A3, "Z3": Z3}
    return A3, memory

def get_cost_value(Y_hat, Y):
    # number of examples
    m = Y_hat.shape[1]
    # small epsilon to avoid log(0) since real sensor data can push
    # predictions to exactly 0.0 or 1.0 after enough training iterations
    eps = 1e-8
    Y_hat = np.clip(Y_hat, eps, 1 - eps)
    # calculation of the cost according to the formula
    cost = -1 / m * (np.dot(Y, np.log(Y_hat).T) + np.dot(1 - Y, np.log(1 - Y_hat).T))
    return np.squeeze(cost)


def sigmoid_backward(dA, Z):
    sig = sigmoid(Z)
    return dA * sig * (1 - sig)

def relu_backward(dA, Z):
    # TODO 5: Implement derivative of relu function
    dZ = None
    return dZ

def single_layer_backward_propagation(dA_curr, W_curr, b_curr, Z_curr, A_prev, activation="relu"):
    """ This function performs single layer back propagation.

    Args:
        dA_curr (np.ndarray): delta A matrix in current layer
        W_curr (np.ndarray): weight matrix in current layer
        b_curr (np.ndarray): bias vector in current layer
        Z_curr (np.ndarray): Z vector stored in current layer
        A_prev (np.ndarray): A matrix in previous layer
        activation (str, optional): defines activation function. Either sigmoid or relu.

    Returns:
        dA_prev (np.ndarray): delta A matrix in previous layer
        dW_curr (np.ndarray): delta Weight matrix in current layer
        db_curr (np.ndarray): delta bias vector in current layer
    """
    # TODO 6: Implement this function.

    # number of examples
    m = A_prev.shape[1]

    if activation == "relu":
        backward_activation_func = relu_backward
    elif activation == "sigmoid":
        backward_activation_func = sigmoid_backward
    else:
        raise Exception('Non-supported activation function')

    # calculation of the activation function derivative
    dZ_curr = None
    # derivative of the matrix W
    dW_curr = None
    # derivative of the vector b
    db_curr = None
    # derivative of the matrix A_prev
    dA_prev = None
    return dA_prev, dW_curr, db_curr


def confusion_matrix(y_true, y_pred):
    """Compute a binary confusion matrix.

    Args:
        y_true (np.ndarray): ground-truth labels, shape (1, m) or (m,), values in {0,1}
        y_pred (np.ndarray): predicted labels, same shape, values in {0,1}

    Returns:
        dict with keys tp, fp, tn, fn
    """
    # TODO 7: implement this function.
    y_true = y_true.flatten().astype(int)
    y_pred = y_pred.flatten().astype(int)

    tp = None
    tn = None
    fp = None
    fn = None
    return {"tp": tp, "fp": fp, "tn": tn, "fn": fn}


def compute_metrics(y_true, y_pred):
    """Compute accuracy, precision, recall and F1 from a confusion matrix.

    This matters on this dataset specifically because "Occupied" is a
    minority class (~21% of rows) — a model that always predicts "not
    occupied" would already score ~79% accuracy while being useless.
    Precision/recall/F1 expose that in a way plain accuracy doesn't.

    Args:
        y_true (np.ndarray): ground-truth labels
        y_pred (np.ndarray): predicted labels (already thresholded to 0/1)

    Returns:
        dict with keys accuracy, precision, recall, f1, confusion_matrix
    """
    # TODO 8: implement this function using confusion_matrix() above.
    cm = confusion_matrix(y_true, y_pred)
    tp, fp, tn, fn = cm["tp"], cm["fp"], cm["tn"], cm["fn"]

    accuracy = None
    precision = None
    recall = None
    f1 = None

    return {
        "accuracy": accuracy,
        "precision": precision,
        "recall": recall,
        "f1": f1,
        "confusion_matrix": cm,
    }
