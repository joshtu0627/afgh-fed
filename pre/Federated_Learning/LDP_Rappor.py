import numpy as np
import random


PRECISION = 4

def PermanentRandomizedResponse(v):
    v = int(v)
    # set by user
    f = 0.5

    prob = np.random.random()
    if prob <= (f / 2):
        v = 1
    elif (f / 2) < prob <= f:
        v = 0
    else:
        v = v
    return str(v)


def InstantaneousRandomizedResponse(v):
    v = int(v)
    # set by user
    q = 0.2
    p = 0.8

    prob = np.random.random()
    if v == 1:
        if prob <= q:
            v = 1
        else:
            v = 0
    elif v == 0:
        if prob <= p:
            v = 1
        else:
            v = 0
    return str(v)


def RAPPOR(value):
    # base10 to base2
    shift_val = int(abs(value * pow(10, PRECISION)))
    value_bin = bin(shift_val)[2:]

    response = ''
    for v in value_bin:
        v = PermanentRandomizedResponse(v)
        v = InstantaneousRandomizedResponse(v)
        response += v


    # base2 to base10
    response = int(response, 2) / pow(10, PRECISION)
    if value < 0:
        response *= -1

    noise = response - value
    return noise

# calculate noises
# noises = {}
# for name in grad.keys():
#     tmp_list = []
#     if 'weight' in name:
#         for i in range(len(grad[name])):
#             tmp = []
#             for j in range(len(grad[name][i])):
#                 tmp.append(RAPPOR(grad[name][i][j]))
#             tmp_list.append(tmp)
#     elif 'bias' in name:
#         for i in range(len(grad[name])):
#             tmp_list.append(RAPPOR(grad[name][i]))

#     noises[name] = torch.Tensor(tmp_list).to(self.args.device)