#---------------------------
# mvg_mechanism.py
# Author: Thee Chanyaswad
#
# Version 1.0
#	- Initial implementation.
#---------------------------

import os
import time
import numpy as np


def compute_precision_budget(m, n, gamma, s2, epsilon, delta):
	maxRank = np.min((n,m))
	harmR1 = _get_harmonic_num(maxRank,1)
	harmR12 = _get_harmonic_num(maxRank,0.5)
	alpha = (harmR1 + harmR12) * (gamma ** 2) + 2*harmR1*gamma*s2
	zeta = 2*np.sqrt(-m*n*np.log(delta))-2*np.log(delta) + m*n
	beta = 2*((m*n)**0.25)*zeta*harmR1*s2
	
	#get total precision budget
	precisionBudget = ((-beta + np.sqrt(beta**2 + 8*alpha*epsilon))**2) / (4*(alpha**2))
	return precisionBudget


def generate_mvg_noise_via_affine_tx(rowCov,colCov):
	n = len(colCov)
	m = len(rowCov)
	mu = np.zeros((n,m))
	sampIid = np.random.normal(0,1,size=mu.shape) # sample iid gauss first
	
	uSig,s,vSig = np.linalg.svd(rowCov)
	sSig = np.diag(np.sqrt(s))
	Bsig = np.inner(uSig,sSig.T)
	uPsi,s,vPsi = np.linalg.svd(colCov)
	sPsi = np.diag(np.sqrt(s))
	Bpsi = np.inner(uPsi,sPsi.T)
	
	#affine tx
	sampMvg = np.inner(np.inner(Bpsi,sampIid.T),Bsig).T
	
	return sampMvg


def generate_mvg_noise_via_multivariate_gaussian(rowCov,colCov):
	cov = np.kron(colCov,rowCov)
	muVec = np.zeros(len(cov))
	sampVect = np.random.multivariate_normal(muVec, cov, size=1)
	sampMvg = sampVect.reshape((len(rowCov),len(colCov)),order='F')
	return sampMvg


def _get_harmonic_num(order,power=1.0):
	if order == 1:
		value = 1.0
	else:
		value = 1.0/(order**power) + _get_harmonic_num(order-1,power=power)
	return value




# for DataSetNo in range(1, 5):
#     if DataSetNo == 1:
#         DataSet = "HeartScale"
#         DataSize_n = 270
#     if DataSetNo == 2:
#         DataSet = "BreastcancerScale"
#         DataSize_n = 683
#     if DataSetNo == 3:
#         DataSet = "DiabetesScale"
#         DataSize_n = 768
#     if DataSetNo == 4:
#         DataSet = "SpliceScale"
#         DataSize_n = 1000

#     DataSize_m = DataSize_n

#EpsilonDegree = 12
#round_RemainFloatingPointNo = 1

DataSet = "BreastcancerScale"
DataSize_n = DataSize_m = 683
for round_RemainFloatingPointNo in range(1 ,4):
    for EpsilonDegree in range(10, 19):        

        # parameters
        gamma = DataSize_n  #sup: "Max" of every value in kernel matrix is 1
        s2 = 2 * DataSize_n 
        epsilon = 10**EpsilonDegree
        delta = 1/DataSize_n

        #init
        fmt_npsavetxt = '%1.' + str(round_RemainFloatingPointNo) + 'f'

        precisionBudget = compute_precision_budget(DataSize_m, DataSize_n, gamma, s2, epsilon, delta)
        p_i = 1/DataSize_m * precisionBudget
        oi =  1/np.sqrt(p_i)
        A_E = np.diag(np.full(DataSize_m, oi))  
        CovMatrix = A_E  #W_E和W_E_T在此皆為I

        #make dir
        path = "./DataGenerated/"+DataSet
        if not os.path.isdir(path):    
            os.mkdir(path)       
        path = "./DataGenerated/"+DataSet+"/"+DataSet+"_round"+str(round_RemainFloatingPointNo)
        if not os.path.isdir(path):    
            os.mkdir(path)         
        path = "./DataGenerated/"+DataSet+"/FinalResult"
        if not os.path.isdir(path):
            os.mkdir(path)

        ############Part 1~3
        for PartNo in range(1, 4):            
            start = time.process_time()                       
            mvg_noise_via_affine_tx_ScaleHeart = generate_mvg_noise_via_affine_tx(CovMatrix, CovMatrix)
            end = time.process_time()
            GenDPNoiseTime = (end - start)             
            np.savetxt("./DataGenerated/"+DataSet+"/"+DataSet+"_round"+str(round_RemainFloatingPointNo)+"/mvg_noise_via_affine_tx_"+DataSet+"_Part"+str(PartNo)+"_Epsn"+str(EpsilonDegree)+"_round"+str(round_RemainFloatingPointNo)+".csv", np.round(mvg_noise_via_affine_tx_ScaleHeart, decimals = round_RemainFloatingPointNo), delimiter=",", fmt = fmt_npsavetxt)    
            #print('precisionBudget=', precisionBudget)
            #print('A_E=', A_E)
            #print('mvg_noise_via_affine_tx_ScaleHeart=', mvg_noise_via_affine_tx_ScaleHeart)


        ####Save File####
        FinalResultDirFile = "./DataGenerated/"+DataSet+"/FinalResult/" + DataSet + "_DP_MVG_FinalResult"
        lines = [str(DataSet) + ',' + str(DataSize_n) + ',' + str(round_RemainFloatingPointNo) + ',' + str(EpsilonDegree) + ',' + str(GenDPNoiseTime)]
        with open(FinalResultDirFile, 'a') as f:
            for line in lines:
                f.write(line)
                f.write('\n')