import numpy as np
import torch
import re


def ReadNoisesSumFile(path):
    noises_sum = []
    with open(path, 'r') as file:
        content = re.split(' |\n', file.read())
        for i in range(len(content)):
            if content[i]:
                noises_sum.append(float(content[i]))
    return noises_sum


def WriteUtilsFile(path, data):
    with open(path, 'w') as file:
        file.write(str(data))
    # EndOfFile


def WriteNoiseFile(path, data):
    with open(path, 'w') as file:
        # calculate total data size
        data_size = 0
        for name in data.keys():
            if 'weight' in name:
                data_size += (data[name].shape[0] * data[name].shape[1])
            elif 'bias' in name:
                data_size += data[name].shape[0]

        # write data size
        # file.write(str(data_size) + '\n')

        # write data items
        for name in data.keys():
            if 'weight' in name:
                for i in range(len(data[name])):
                    for j in range(len(data[name][i])):
                        file.write(str(data[name][i][j].item()) + ' ')
            elif 'bias' in name:
                for i in range(len(data[name])):
                    file.write(str(data[name][i].item()) + ' ')
    # EndOfFile


def WriteNoiseTotalFile(path, data):
    with open(path, 'w') as file:
        for name in data.keys():
            if 'weight' in name:
                for i in range(len(data[name])):
                    for j in range(len(data[name][i])):
                        file.write(str(data[name][i][j].item()) + ' ')
            elif 'bias' in name:
                for i in range(len(data[name])):
                    file.write(str(data[name][i].item()) + ' ')
    # EndOfFile
