import numpy as np
import csv
import re

noises_list = []
for i in range(6):
    with open('./clients_noises/client' + str(i) + '.txt', 'r') as file:
        content = re.split(' |\n', file.read())
        noises_size = int(content[0])
        noises = []
        for j in range(noises_size):
            noises.append(float(content[j+1]))
        noises_list.append(noises)

noises_sum = []
for j in range(noises_size):
    sum = 0
    for i in range(6):
        sum += noises_list[i][j]
    noises_sum.append(sum)
print(noises_sum)        