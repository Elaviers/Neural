#include "NetPerceptron.hpp"

NetPerceptron::InputPair* NetPerceptron::GetInput(NetPerceptron* perceptron)
{
	for (InputPair& input : _inputs)
		if (input.perceptron == perceptron)
			return &input;

	return nullptr;
}

bool NetPerceptron::RemoveInput(NetPerceptron* perceptron)
{
	bool success = false;

	for (auto it = _inputs.begin(); it.IsValid();)
	{
		if (it->perceptron = perceptron)
		{
			it = _inputs.Remove(it);
			success = true;
		}
		else ++it;
	}

	if (success) Invalidate();
	return success;
}

void NetPerceptron::UpdateValidity()
{
	if (_state == State::Invalid)
		return;

	for (InputPair& i : _inputs)
	{
		if (i.perceptron->_state == State::Invalid)
		{
			_state = State::Invalid;
			return;
		}

		i.perceptron->UpdateValidity();
		if (i.perceptron->_state == State::Invalid)
		{
			_state = State::Invalid;
			return;
		}
	}
}

bool NetPerceptron::Evaluate()
{
	if (_state != State::Invalid)
		return _state == State::True;

	float sum = _bias;

	for (const InputPair& i : _inputs)
	{
		if (i.perceptron->Evaluate())
			sum += i.weight;
	}

	bool b = sum > 0.f;
	_state = b ? State::True : State::False;
	return b;
}
