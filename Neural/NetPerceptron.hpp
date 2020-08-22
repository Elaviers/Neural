#pragma once
#include <ELCore/List.hpp>

class NetPerceptron
{
public:
	struct InputPair
	{
		NetPerceptron* perceptron;
		float weight;
	};

private:
	float _bias;
	List<InputPair> _inputs;

	enum class State { Invalid, False, True } _state;

public:
	NetPerceptron() : _bias(0.f), _state(State::Invalid) {}
	virtual ~NetPerceptron() {}

	float GetBias() const { return _bias; }
	const List<const InputPair>& GetOutputs() const { return (const List<const InputPair>&)_inputs; }
	
	void SetBias(float bias) { if (_bias != bias) Invalidate(); _bias = bias; }

	void AddInput(NetPerceptron* output, float weight) { _inputs.Add({ output, weight }); Invalidate(); }
	InputPair* GetInput(NetPerceptron*);
	bool RemoveInput(NetPerceptron*);
	void ClearInputs() { _inputs.Clear(); Invalidate(); }

	void UpdateValidity();
	bool Evaluate();

	void Invalidate() { _state = State::Invalid; }
};
