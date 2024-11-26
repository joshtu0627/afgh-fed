import cv2
import numpy as np

original_images = []
reversed_images = []
accuracy = []
for epsilon in range(-6, 7):
    for i in range(10):
        img = cv2.imread('./Experiment/weight&bias/pictures/ep_'+str(epsilon)+'/original'+str(i)+'.jpg')
        original_images.append(img)

    for i in range(200):
        img = cv2.imread('./Experiment/weight&bias/pictures/ep_'+str(epsilon)+'/reversed'+str(i)+'.jpg')
        reversed_images.append(img)

    max_correct = 0
    for i in range(len(original_images)):
        for j in range(len(reversed_images)):
            curr_correct = np.sum(np.equal(original_images[i], reversed_images[j]))
            if curr_correct > max_correct:
                max_correct = curr_correct

    accuracy.append(round(max_correct / original_images[0].size * 100))

    print('epsilon = ' + str(epsilon) + '\nbest score = ' + str(accuracy[-1]) + '%\n')

