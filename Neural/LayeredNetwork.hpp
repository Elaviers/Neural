#pragma once
#include <ELCore/Buffer.hpp>
#include <ELCore/Concepts.hpp>
#include <ELCore/List.hpp>

/*
	Currently intended for use as a shallow network only!

	Uses the quadratic cost function & the sigmoid activation function
	Training is done by solving the weights and biases partial derivatives in terms of the cost function via backpropogation
*/

class LayeredNetwork
{
public:
	class Layer;

	enum class LinkingType
	{
		NONE = 0,
		ALL = 1 //Link to every node in previous layer
	};

private:
	class Neuron
	{
		friend LayeredNetwork;
		friend Layer;

	public:
		struct Weight
		{
			double value;

			double pd_C; //Partial derivative with respect to cost
		};

	private:
		bool _valid;

		double _bias;
		double _output;

		LinkingType _linkType;

		int _inputLayer;
		Buffer<Weight> _inputWeights;

		double _input;

		double _error;
		double _bias_pdC; //nabla bias / nabla cost

	public:
		Neuron() : 
			_valid(false), 
			_bias(0.0), 
			_output(0.0), 
			_inputLayer(-1),
			_linkType(LinkingType::NONE), 
			_input(0.0),
			_error(0.0),
			_bias_pdC(0.0) {}

		double Evaluate(LayeredNetwork&, size_t layer);
	};

public:
	class Layer
	{
		friend LayeredNetwork;
		friend Buffer<Layer>; //EW!

	private:
		LayeredNetwork* _network;

		LinkingType _linkType;
		Buffer<Neuron> _neurons;

		Layer() : _network(nullptr), _linkType(LinkingType::NONE) {}
	public:
		size_t GetSize() const { return _neurons.GetSize(); }

		void Generate(size_t size) 
		{
			_neurons.Clear();
			_neurons.SetSize(size);

			if (_linkType != LinkingType::NONE)
				SetInputLinkType(_linkType);
		}

		void SetInputLinkType(LinkingType linkType);
		void RandomiseWeightsAndBiases(class Random&);
	};

private:
	Buffer<Layer> _layers;

	int _trainSamples;

public:
	LayeredNetwork();
	LayeredNetwork(ByteReader&);

	bool Write(ByteWriter&);

	Layer& CreateLayer() { 
		Layer& l = _layers.Emplace();
		l._network = this;
		return l; 
	}

	Layer& InputLayer() { return _layers[0]; }
	Layer& OutputLayer() { return _layers[1]; }
	Layer& MidLayer(uint32 index) { return _layers[2 + index]; }

	//inputCount must equal input neuron count
	//outputCount must equal output neuron count
	bool Evaluate(
		const double* inputs, size_t inputCount, 
		double* outputs, size_t outputCount);

	void BeginTraining();

	bool Train(
		const double* inputs, size_t inputCount,
		const double* desiredOutputs, double* outputs, size_t outputCount);

	void ApplyTraining(double learningRate);
};
