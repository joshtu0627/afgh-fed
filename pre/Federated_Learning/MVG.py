#---------------------------
# mvg_mechanism.py
# Author: Thee Chanyaswad
#
# Version 1.0
#	- Initial implementation.
#---------------------------



import numpy as np
import torch

PRECISION = 4

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

def generate_mvg(grad, epsilon, device):
	noises = {}
	for name in grad.keys():
		# parameters
		if 'weight' in name:
			data_size_m = grad[name].shape[0]
			data_size_n = grad[name].shape[1]
		elif 'bias' in name:
			data_size_m = grad[name].shape[0]
			data_size_n = 1
		gamma = torch.max(grad[name]).item()
		if gamma == 0:gamma += 1 # gamma can not be 0
		sensitivity = (torch.max(grad[name]) - torch.min(grad[name])).item()
		epsilon = epsilon
		delta = 1 / (data_size_m * data_size_n)

		# calculate algo
		precision_budget = compute_precision_budget(data_size_m, data_size_n, gamma, sensitivity, epsilon, delta)
		pi = (1) * precision_budget
		oi =  1 / np.sqrt(pi)
		if np.isnan(oi):oi = 0
		CovMatrix_m = np.diag(np.full(data_size_m, oi))
		CovMatrix_n = np.diag(np.full(data_size_n, oi))  
		noises[name] = generate_mvg_noise_via_affine_tx(CovMatrix_m, CovMatrix_n)
		noises[name] = torch.from_numpy(noises[name]).to(device)
		noises[name] = torch.round(noises[name], decimals=PRECISION)
		if 'bias' in name:
			noises[name] = torch.flatten(noises[name])

	return noises