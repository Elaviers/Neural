#pragma once
#include <ELCore/List.hpp>
#include <exception>

class InvalidInputCountException : public std::exception
{
	virtual const char* what() const noexcept override
	{
		return "Invalid input count! Must be 0, 1 or weight count.";
	}
};

/*
	Y = {1 if W*X+B > 0 | 0 if W*X+B <= 0}
*/
class Perceptron
{
private:
	size_t _weightCount;
	List<float> _weights;
	float _bias;

public:
	Perceptron(float bias = 0.f) : _weightCount(0), _bias(bias) {}
	~Perceptron() {}

	float GetWeightCount() const { return _weightCount; }
	float GetBias() const { return _bias; }

	void SetBias(float bias) { _bias = bias; }

	/*
		-1 = add to end
	*/
	bool AddInput(float weight, int index = -1);
	bool Evaluate(const bool* inputs, size_t inputCount) const;
};
