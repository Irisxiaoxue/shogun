/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2011 Alesis Novik
 * Copyright (C) 2011 Berlin Institute of Technology and Max-Planck-Society
 */

#include "distributions/Gaussian.h"
#include "lib/Mathematics.h"
#include "base/Parameter.h"

using namespace shogun;

CGaussian::CGaussian() : CDistribution()
{
	float64_t* mean = new float64_t[1];
	float64_t* cov = new float64_t[1];
	mean[0] = 0;
	cov[0] = 1;
	int32_t dim = 1;
	init(mean, cov, dim);
	init();
}

CGaussian::CGaussian(float64_t* mean, float64_t* cov, int32_t dim) : CDistribution()
{
	init(mean, cov, dim);
	init();
}

void CGaussian::init(float64_t* mean, float64_t* cov, int32_t dim)
{
	m_mean = new float64_t[dim];
	memcpy(m_mean, mean, sizeof(float64_t)*dim);
	m_cov = new float64_t[dim*dim];
	memcpy(m_cov, cov, sizeof(float64_t)*dim*dim);
	m_cov_inverse = new float64_t[dim*dim];
	memcpy(m_cov_inverse, cov, sizeof(float64_t)*dim*dim);
	int32_t result = clapack_dpotrf(CblasRowMajor, CblasLower, dim, m_cov_inverse, dim);
	m_constant = 1;

	for (int i = 0; i < dim; i++)
		m_constant *= m_cov_inverse[i*dim+i];

	m_constant = 1/m_constant;
	m_constant *= pow(2*M_PI, (float64_t) -dim/2);

	result = clapack_dpotri(CblasRowMajor, CblasLower, dim, m_cov_inverse, dim);
	m_dim = dim;
}

CGaussian::~CGaussian()
{	
	SG_UNREF(m_data);
	delete[] m_cov_inverse;
	delete[] m_cov;
	delete[] m_mean;
}

bool CGaussian::train(CFeatures* data)
{
	// init features with data if necessary and assure type is correct
	if (data)
	{
		if (!data->has_property(FP_DOT))
				SG_ERROR("Specified features are not of type CDotFeatures\n");		
		set_data((CDotFeatures*) data);
	}

	float64_t* mean;
	float64_t* cov;
	int32_t dim;

	m_data->get_mean_cov(&mean, &cov, &dim);

	init(mean, cov, dim);
	
	return true;
}

int32_t CGaussian::get_num_model_parameters()
{
	return m_dim*(m_dim+1);
}

float64_t CGaussian::get_log_model_parameter(int32_t num_param)
{
	if (num_param<m_dim)
		return CMath::log(m_mean[num_param]);
	else
		return CMath::log(m_cov[num_param-m_dim]);
}

float64_t CGaussian::get_log_derivative(int32_t num_param, int32_t num_example)
{
	return 0;
}

float64_t CGaussian::get_log_likelihood_example(int32_t num_example)
{
	float64_t* point;
	int32_t point_len;
	m_data->get_feature_vector(&point, &point_len, num_example);
	return CMath::log(compute_PDF(point, point_len));
}

float64_t CGaussian::compute_PDF(float64_t* point, int32_t point_len)
{
	ASSERT(point_len == m_dim);
	float64_t* difference = new float64_t[m_dim];
	memcpy(difference, point, sizeof(float64_t)*m_dim);
	float64_t* result = new float64_t[m_dim];

	for (int i = 0; i < m_dim; i++)
		difference[i] -= m_mean[i];

	cblas_dsymv(CblasRowMajor, CblasLower, m_dim, -1.0/2.0, m_cov_inverse, m_dim,
				difference, 1, 0, result, 1);
	return m_constant * exp(cblas_ddot(m_dim, difference, 1, result, 1));
}

void CGaussian::init()
{
	m_parameters->add_matrix(&m_cov, &m_dim, &m_dim, "m_cov", "Covariance.");
	m_parameters->add_matrix(&m_cov_inverse, &m_dim, &m_dim, "m_cov_inverse", "Covariance inverse.");
	m_parameters->add_vector(&m_mean, &m_dim, "m_mean", "Mean.");
	m_parameters->add(&m_dim, "m_dim", "Dimensionality.");
	m_parameters->add(&m_constant, "m_constant", "Constant part.");
}
