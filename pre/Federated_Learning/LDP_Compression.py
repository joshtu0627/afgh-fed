import numpy as np

# parameters
L = 8 # disjoint gradients into L groups
k = 4 # select top k gradients
B = 0.5
C = 0.1
b = 0.066 # scale = sensitivity / epsilon

def Laplace_Mechanism(scale):
    # scale = sensitivity / epsilon
    noise = np.random.laplace(0, scale)
    return noise


def DP_MEMSGD(grad, lr, e):
    grad_list = []
    for name in grad.keys():
        if 'weight' in name:
            for i in range(len(grad[name])):
                grad_list += grad[name][i]
        elif 'bias' in name:
            grad_list += grad[name]

    p = []
    for g in grad_list:
        p.append(lr * g + e)

    m = int(len(grad_list) / L) # divided into L groups with m members
    if k > m:
        k = m

    delta = []
    for j in range(L):
        start = j * m
        end = j * m + m if j != L-1 else len(grad_list)
        top_k = k
        for i in range(start, end):
            if top_k > 0:
                delta.append(p[i] / max(0.5 + B, abs(p[i]) / C) + Laplace_Mechanism(b))
                top_k -= 1
            else:
                delta.append(0)
    e = p - delta
    return delta, e
    