#include "LayeredNetwork.hpp"
#include <ELCore/ByteReader.hpp>
#include <ELCore/ByteWriter.hpp>
#include <ELMaths/Maths.hpp>
#include <ELMaths/Random.hpp>
#include <ELSys/Debug.hpp>

__forceinline double Activate(double x)
{
	//sigmoid 
	return 1.0 / (1.0 + Maths::Exp(-x));
}

double ActivatePrime(double x)
{
	//first derivative of sigmoid
	double sig = Activate(x);
	return sig * (1.0 - sig);
}

double LayeredNetwork::Neuron::Evaluate(LayeredNetwork& network, size_t layer)
{
	if (_valid) return _output;

	_input = _bias;

	if (_linkType == LinkingType::ALL)
	{
		if (layer > 0)
		{
			Buffer<Neuron>& inputNeurons = network._layers[_inputLayer]._neurons;
			for (int i = 0; i < _inputWeights.GetSize(); ++i)
				_input += _inputWeights[i].value * inputNeurons[i].Evaluate(network, _inputLayer);
		}
	}

	if (layer == 0) Debug::PrintLine("LayeredNetwork Warning: input layer being evaluated while invalid!");

	_valid = true;
	return _output = Activate(_input);
}

void LayeredNetwork::Layer::SetInputLinkType(LinkingType linkType)
{
	_linkType = linkType;

	if (linkType == LinkingType::ALL)
	{
		int prevLayer = -1;
		
		for (int i = 0; i < _network->_layers.GetSize() - 1; ++i)
			if (&_network->_layers[i + 1] == this)
			{
				prevLayer = i;
				break;
			}

		if (prevLayer >= 0)
		{
			int inputLayer = -1;

			if (prevLayer == 0)
			{
				//we are the output layer

				if (_network->_layers.GetSize() > 2)
					inputLayer = _network->_layers.GetSize() - 1;

			}
			else if (prevLayer == 1)
			{
				//we are the first mid layer

				inputLayer = 0;
			}
			else
				inputLayer = prevLayer;

			if (inputLayer >= 0)
			{
				uint32 inputCount = _network->_layers[inputLayer]._neurons.GetSize();

				for (Neuron& neuron : _neurons)
				{
					neuron._linkType = _linkType;
					neuron._inputLayer = inputLayer;
					neuron._inputWeights.Clear();
					neuron._inputWeights.SetSize(inputCount);
				}

				return;
			}
		}
	}
		
	for (Neuron& neuron : _neurons)
	{
		neuron._linkType = LinkingType::NONE;
		neuron._inputLayer = -1;
		neuron._inputWeights.Clear();
	}
}

void LayeredNetwork::Layer::RandomiseWeightsAndBiases(Random& random)
{
	for (Neuron& neuron : _neurons)
	{
		neuron._bias = random.NextDouble() * 2.0 - 1.0;

		for (size_t i = 0; i < neuron._inputWeights.GetSize(); ++i)
			neuron._inputWeights[i].value = random.NextDouble() * 2.0 - 1.0;
	}
}

LayeredNetwork::LayeredNetwork() : _trainSamples(0)
{
	//todo jank
	_layers.SetSize(2);

	_layers[0]._network = this;
	_layers[1]._network = this;
}

LayeredNetwork::LayeredNetwork(ByteReader& reader) : _trainSamples(0)
{
	uint32 version = reader.Read_uint32();

	std::cout << "Reading netfile...\n";

	if (version != 1)
	{
		Debug::Error("Invalid netfile");
		throw 1;
	}

	uint32 layerCount = reader.Read_uint32();
	std::cout << "LayerCount: " << layerCount << '\n';

	if (layerCount < 2)
	{
		Debug::Error("Invalid netfile (degenerate layer count)");
		throw 2;
	}

	_layers.SetSize(layerCount);

	for (uint32 i = 0; i < layerCount; ++i)
	{
		_layers[i]._network = this;
		_layers[i]._neurons.SetSize(reader.Read_uint32());
		
		std::cout << "Layer " << i << ": " << _layers[i]._neurons.GetSize() << " neurons\n";
	}

	for (Layer& layer : _layers)
		for (Neuron& neuron : layer._neurons)
		{
			neuron._bias = reader.Read_double();
			neuron._linkType = (LinkingType)reader.Read_uint16();

			if (neuron._linkType == LinkingType::ALL)
			{
				neuron._inputLayer = reader.Read_uint32() - 1;

				neuron._inputWeights.SetSize(reader.Read_uint32());
				for (uint32 i = 0; i < neuron._inputWeights.GetSize(); ++i)
					neuron._inputWeights[i].value = reader.Read_double();
			}
		}

	std::cout << "Done\n";
}

bool LayeredNetwork::Write(ByteWriter& writer)
{
	writer.Write_uint32(1);

	uint32 layerCount = _layers.GetSize();
	writer.Write_uint32(layerCount); //Layer count (incl input and output)

	std::cout << "Writing netfile...\nLayerCount: " << layerCount << "\n";

	int i = 0;
	for (Layer& layer : _layers)
	{
		writer.Write_uint32(layer._neurons.GetSize());

		std::cout << "Layer " << i++ << ": " << layer._neurons.GetSize() << " neurons\n";
	}

	std::cout << "Writing neuron info...\n";
	
	int li = 0;
	for (Layer& layer : _layers)
	{
		size_t ensure = layer._neurons.GetSize() * (8 + 2);
		
		for (const Neuron& neuron : layer._neurons)
		{
			if (neuron._linkType == LinkingType::ALL)
			{
				ensure += (4 * 2) + 8 * neuron._inputWeights.GetSize();
			}
		}
		
		writer.EnsureSpace(ensure);
		std::cout << "LAYER " << li++ << ": " << ensure << " bytes...\n";

		for (const Neuron& neuron : layer._neurons)
		{
			writer.Write_double(neuron._bias);
			writer.Write_uint16((uint16)neuron._linkType);

			if (neuron._linkType == LinkingType::ALL)
			{
				if (neuron._inputLayer < -1)
				{
					Debug::Error("Invalid network: neuron has invalid inputLayer");
					return false;
				}

				writer.Write_uint32(neuron._inputLayer + 1);

				writer.Write_uint32(neuron._inputWeights.GetSize());
				for (uint32 i = 0; i < neuron._inputWeights.GetSize(); ++i)
					writer.Write_double(neuron._inputWeights[i].value);
			}
		}
	}

	std::cout << "Done\n";
	return true;
}

bool LayeredNetwork::Evaluate(const double* inputs, size_t inputCount, double* outputs, size_t outputCount)
{
	if (inputCount != _layers[0]._neurons.GetSize() || outputCount != _layers[1]._neurons.GetSize())
		return false;

	for (Layer& layer : _layers)
		for (Neuron& neuron : layer._neurons)
			neuron._valid = false;

	for (size_t i = 0; i < inputCount; ++i)
	{
		_layers[0]._neurons[i]._output = inputs[i];
		_layers[0]._neurons[i]._valid = true;
	}

	for (size_t i = 0; i < outputCount; ++i)
		outputs[i] = OutputLayer()._neurons[i].Evaluate(*this, 1);

	return true;
}

void LayeredNetwork::BeginTraining()
{
	for (Layer& layer : _layers)
		for (Neuron& neuron : layer._neurons)
		{
			neuron._bias_pdC = 0.0;
			for (size_t i = 0; i < neuron._inputWeights.GetSize(); ++i)
				neuron._inputWeights[i].pd_C = 0.0;
		}

	_trainSamples = 0;
}

bool LayeredNetwork::Train(const double* inputs, size_t inputCount, const double* desiredOutputs, double* outputs, size_t outputCount)
{
	if (inputCount != _layers[0]._neurons.GetSize() || outputCount != _layers[1]._neurons.GetSize())
		return false;

	for (Layer& layer : _layers)
		for (Neuron& neuron : layer._neurons)
		{
			neuron._valid = false;
			neuron._error = 0.0;
		}

	for (size_t i = 0; i < inputCount; ++i)
	{
		_layers[0]._neurons[i]._output = inputs[i];
		_layers[0]._neurons[i]._valid = true;
	}

	for (size_t i = 0; i < outputCount; ++i)
	{
		Neuron& oNeuron = _layers[1]._neurons[i];
		outputs[i] = oNeuron.Evaluate(*this, 1);

		//error on the output layer = partial derivative of cost function in terms of the input * derivative of activation function
		//This will be multiplied by activation prime later
		oNeuron._error = oNeuron._output - desiredOutputs[i];
	}

	//Calculate weight and bias PDs for each layer except input
	int layer = 1;
	while (layer > 0)
	{
		int nextLayer = -1;

		for (Neuron& neuron : _layers[layer]._neurons)
		{
			if (neuron._inputLayer != nextLayer)
			{
				if (nextLayer >= 0)
				{
					Debug::Error("Training does not support layers which take input from more than one layer!");
					return false;
				}

				nextLayer = neuron._inputLayer;
			}

			neuron._error *= ActivatePrime(neuron._input);
			neuron._bias_pdC += neuron._error;

			Buffer<Neuron>& inputNeurons = _layers[neuron._inputLayer]._neurons;
			for (size_t input = 0; input < neuron._inputWeights.GetSize(); ++input)
			{
				//Add weighted error to input error
				inputNeurons[input]._error += neuron._error * neuron._inputWeights[input].value;

				//Weight PD
				neuron._inputWeights[input].pd_C += inputNeurons[input]._output * neuron._error;
			}
		}

		layer = nextLayer;
	}

	++_trainSamples;
	return true;
}

void LayeredNetwork::ApplyTraining(double learningRate)
{
	if (_trainSamples <= 0) return;

	double f = learningRate / (double)_trainSamples;

	for (size_t layer = 1; layer < _layers.GetSize(); ++layer)
		for (Neuron& neuron : _layers[layer]._neurons)
		{
			neuron._bias -= f * neuron._bias_pdC;

			for (size_t i = 0; i < neuron._inputWeights.GetSize(); ++i)
				neuron._inputWeights[i].value -= f * neuron._inputWeights[i].pd_C;
		}
}
