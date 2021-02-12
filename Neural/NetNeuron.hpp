#pragma once
#pragma once
#include <ELCore/List.hpp>

class NetNeuron
{
public:
	struct InputPair
	{
		NetNeuron* perceptron;
		float weight;
	};

private:
	float _bias;
	List<InputPair> _inputs;

	float _value;
	bool _valid;

public:
	NetNeuron() : _bias(0.f), _value(-1.f), _valid(false) {}
	virtual ~NetNeuron() {}

	float GetBias() const { return _bias; }
	const List<const InputPair>& GetInputs() const { return (const List<const InputPair>&)_inputs; }

	void SetBias(float bias) { if (_bias != bias) Invalidate(); _bias = bias; }

	void AddInput(NetNeuron* input, float weight) { _inputs.Add({ input, weight }); Invalidate(); }
	bool RemoveInput(NetNeuron*);
	void ClearInputs() { _inputs.Clear(); Invalidate(); }
	
	void UpdateValidity();
	float Evaluate();

	void Invalidate() { _valid = false; }
};


