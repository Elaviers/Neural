#include "Perceptron.hpp"

bool Perceptron::AddInput(float weight, int index)
{
	bool success = true;
	if (index >= 0)
		success = _weights.Insert(index, weight).IsValid();
	else
		_weights.Add(weight);

	if (success)
		++_weightCount;

	return success;
}

bool Perceptron::Evaluate(const bool* inputs, size_t inputCount) const
{
	static InvalidInputCountException invalidInputCountException;

	if (inputCount == 0)
		return _bias > 0.f;

	if (inputCount == 1)
	{
		if (inputs[0])
		{
			float sum = _bias;
			for (float w : _weights)
				sum += w;

			return sum > 0.f;
		}

		return _bias > 0.f;
	}

	if (inputCount == _weightCount)
	{
		float sum = _bias;
		int i = 0;
		for (float w : _weights)
			if (inputs[i++])
				sum += w;

		return sum > 0.f;
	}

	throw invalidInputCountException;
}
