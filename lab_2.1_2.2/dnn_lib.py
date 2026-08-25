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
    # TODO 3: implement this method.
    norm_data = None
    max_values = None
    return norm_data, max_values

def data_normalize_prediction(raw_data, max_values):
    norm_data=(raw_data - raw_data.min(axis=0))/(raw_data.max(axis=0)- raw_data.min(axis=0))
    return norm_data

def sigmoid(Z):
    return 1 / (1 + np.exp(-Z))

def relu(Z):
    # TODO 4: implement relu function.
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
    # TODO 5: implement this function.
    # calculation of the input value for the activation function
    Z_curr = None

    # selection of activation function
    if activation is "relu":
        activation_func = relu
    elif activation is "sigmoid":
        activation_func = sigmoid
    else:
        raise Exception('Non-supported activation function')

    # return of calculated activation A and the intermediate Z matrix
    A_curr = None
    return A_curr, Z_curr

def full_forward_propagation(X, params_values):
    """This function perform full forward propagation using given input vector X and param_values that stores vector of weights and biases.

    Args:
        X (np.ndarray): input vector X
        params_values (_type_): weight and bias vector stored in a dictionary 

    Returns:
        A3: output of the network
        memory: matrix Z and A of each hidden layer, stored in list format 
    """
    # TODO 6: implement this method.
    # You need to call 3 times single_layer_forward_propagation() with correct parameters and then create a memory list with all intermediate matrix values A1, Z1, A2, Z2, A3, Z3 and return it.
    
    A1, Z1 = None
    A2, Z2 = None
    A3, Z3 = None
    
    memory = [
    {"A1": A1},
    {"Z1": Z1},
    {"A2": A2},
    {"Z2": Z2},
    {"A3": A3},
    {"Z3": Z3},
    ]
    return A3, memory

def get_cost_value(Y_hat, Y):
    # number of examples
    m = Y_hat.shape[1]
    # calculation of the cost according to the formula
    cost = -1 / m * (np.dot(Y, np.log(Y_hat).T) + np.dot(1 - Y, np.log(1 - Y_hat).T))
    return np.squeeze(cost)


def sigmoid_backward(dA, Z):
    sig = sigmoid(Z)
    return dA * sig * (1 - sig)

def relu_backward(dA, Z):
    # TODO 8: Implement derivative of relu function
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
    # TODO 9: Implement this function.
    
    # number of examples
    m = A_prev.shape[1]

    # selection of activation function
    if activation is "relu":
        backward_activation_func = relu_backward
    elif activation is "sigmoid":
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
