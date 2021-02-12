#include "NetNeuron.hpp"
#include <ELMaths/Maths.hpp>

bool NetNeuron::RemoveInput(NetNeuron* neuron)
{
	bool success = false;

	for (auto it = _inputs.begin(); it.IsValid();)
	{
		if (it->perceptron = neuron)
		{
			it = _inputs.Remove(it);
			success = true;
		}
		else ++it;
	}

	if (success) Invalidate();
	return success;
}

void NetNeuron::UpdateValidity()
{
	if (!_valid)
		return;

	for (InputPair& i : _inputs)
	{
		i.perceptron->UpdateValidity();
		if (!i.perceptron->_valid)
		{
			Invalidate();
			return;
		}
	}
}

float NetNeuron::Evaluate()
{
	if (_valid) return _value;

	float sum = _bias;
	for (const InputPair& i : _inputs)
		sum += i.weight * i.perceptron->Evaluate();

	_value = 1.f / (1.f + Maths::Exp(-sum));
	_valid = true;
	return _value;
}
