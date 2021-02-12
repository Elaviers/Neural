#include "Digits.hpp"
#include "ImagesIDX3.hpp"
#include "LabelsIDX1.hpp"
#include <ELCore/ByteReader.hpp>
#include <ELCore/ByteWriter.hpp>
#include <ELGraphics/RenderEntry.hpp>
#include <ELGraphics/Texture.hpp>
#include <ELSys/Debug.hpp>
#include <ELSys/IO.hpp>
#include <ELSys/Time.hpp>
#include <ELMaths/Random.hpp>

bool _ReadNetStateFromFile(LayeredNetwork& network)
{
	if (!IO::FileExists("Data/net-state.bin"))
	{
		std::cout << "No existing network found...\n";
		return false;
	}
	else
	{
		std::cout << "Reading net state...\n";
		Buffer<byte> netStateData = IO::ReadFile("Data/net-state.bin", true);

		try
		{
			ByteReader reader(netStateData);
			network = LayeredNetwork(reader);
		}
		catch (int errorCode)
		{
			Debug::Error(CSTR("Netfile read threw ", errorCode));
			return false;
		}
	}

	return true;
}

void _InitTexEnvironment(Texture& tex, Buffer<byte>& imgData, int w, int h)
{
	imgData.SetSize((size_t)w * h * 4);
	for (int i = 3; i < imgData.GetSize(); i += 4)
		imgData[i] = 255; //alpha

	tex.info.aniso = 1;
	tex.info.mipLevels = 1;
	tex.info.minFilter = tex.info.magFilter = GL_NEAREST;
	tex.Create(imgData.Data(), w, h);
}

void _RenderTextureToWindow(Window& window, const GLProgram& program, const Texture& tex, const MeshManager* meshes, const TextureManager* textures)
{
	RenderEntry e(ERenderChannels::ALL);
	e.AddSetTransform(Matrix4::Identity());
	e.AddSetMat4(RCMDSetMat4::Type::PROJECTION, Matrix4::ProjectionOrtho(-.5f, .5f, -.5f, .5f, -1.f, 1.f, 1.f));
	e.AddSetMat4(RCMDSetMat4::Type::VIEW, Matrix4::Identity());
	e.AddSetTexture(tex, 0);
	e.AddSetColour(Colour::White);
	e.AddCommand(RCMDRenderMesh::PLANE);

	RenderContext ctx = { meshes, textures};

	program.Use();
	e.Render(ctx);
	window.SwapBuffers();
}

enum class DrawingUpdate
{
	NOTHING = 0,
	EXIT = 0x1,
	STROKE = 0x2,
	SUBMIT = 0x4
};

DEFINE_BITMASK_FUNCS(DrawingUpdate, int)

DrawingUpdate _HandleDrawingWindow(Window& window, Texture& tex, Buffer<byte>& imgData, bool& drawing, float penRadiusSq)
{
	DrawingUpdate result = DrawingUpdate::NOTHING;
	WindowEvent e;
	while (window.PollEvent(e))
	{
		switch (e.type)
		{
		case WindowEvent::CLOSED:
			window.Hide();
			result |= DrawingUpdate::EXIT;
			break;

		case WindowEvent::RIGHTMOUSEUP:
			//clear image
			for (size_t i = 0; i < imgData.GetSize(); i += 4)
				imgData[i] = imgData[i + 1] = imgData[i + 2] = 0;

			break;

		case WindowEvent::LEFTMOUSEDOWN:
			drawing = true;
			break;

		case WindowEvent::FOCUS_LOST:
			drawing = false;
			break;

		case WindowEvent::LEFTMOUSEUP:
			if (drawing)
			{
				drawing = false;
				result |= DrawingUpdate::STROKE;
			}
			break;

		case WindowEvent::KEYDOWN:
			if (e.data.keyDown.key == EKeycode::ENTER)
				result |= DrawingUpdate::SUBMIT;
			break;

		case WindowEvent::MOUSEMOVE:
			if (drawing)
			{
				Vector2T<uint16> client = window.GetClientSize();
				float mX = (float)e.data.mouseMove.x * (float)tex.GetWidth() / (float)client.x;
				float mY = (float)e.data.mouseMove.y * (float)tex.GetHeight() / (float)client.y;

				for (int y = 0; y < tex.GetWidth(); ++y)
					for (int x = 0; x < tex.GetHeight(); ++x)
					{
						float dx = x + .5f - mX;
						float dy = y + .5f - mY;
						float rsq = dx * dx + dy * dy;

						if (rsq < penRadiusSq)
						{
							byte amount = 255 - (byte)((rsq / penRadiusSq) * 255.f);

							byte* pixel = &imgData[4 * (y * tex.GetWidth() + x)];
							if (*pixel < amount)
							{
								pixel[0] = pixel[1] = pixel[2] = amount;
							}
						}
					}
			}
			break;

		case WindowEvent::RESIZE:
			glViewport(0, 0, e.data.resize.w, e.data.resize.h);
			break;
		}
	}

	return result;
}

const double desiredStates[10][10] = {
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
};

void Digits::Train(int iterations, int batchSize, int layerSize, double learningRate, bool debug)
{
	std::cout << "Begin training for " << iterations << " iterations\nbatch size = " << batchSize << 
		"\nlayer size = " << layerSize << "\nlearning rate = " << learningRate << "\n\n";

	Buffer<byte> trainImageData = IO::ReadFile("Data/train-images.idx3-ubyte");
	Buffer<byte> trainLabelData = IO::ReadFile("Data/train-labels.idx1-ubyte");
	Buffer<byte> testImageData = IO::ReadFile("Data/test-images.idx3-ubyte");
	Buffer<byte> testLabelData = IO::ReadFile("Data/test-labels.idx1-ubyte");

	if (trainImageData.GetSize() <= 0 || trainLabelData.GetSize() <= 0 || testImageData.GetSize() <= 0 || testLabelData.GetSize() <= 0)
		return;

	Random rand(Time::GetRandSeed());

	std::cout << "Creating network...\n";

	if (layerSize < 0)
	{
		if (!_ReadNetStateFromFile(_network)) return;
	}
	else
	{
		_network.InputLayer().Generate(784);
		_network.OutputLayer().Generate(10);

		auto& mid = _network.CreateLayer();
		mid.Generate(layerSize);

		mid.SetInputLinkType(LayeredNetwork::LinkingType::ALL);
		_network.OutputLayer().SetInputLinkType(LayeredNetwork::LinkingType::ALL);

		mid.RandomiseWeightsAndBiases(rand);
		_network.OutputLayer().RandomiseWeightsAndBiases(rand);
	}

	std::cout << "Reading training/testing data...\n";

	ImagesIDX3 trainImages;
	LabelsIDX1 trainLabels;
	ImagesIDX3 testImages;
	LabelsIDX1 testLabels;

	{
		ByteReader trainImageReader(trainImageData);
		if (!trainImages.Read(trainImageReader)) return;
	
		ByteReader trainLabelReader(trainLabelData);
		if (!trainLabels.Read(trainLabelReader)) return;
	
		ByteReader testImageReader(testImageData);
		if (!testImages.Read(testImageReader)) return;

		ByteReader testLabelReader(testLabelData);
		if (!testLabels.Read(testLabelReader)) return;
	}

	Debug::Assert(trainImages.GetHeight() == testImages.GetHeight(), "training / test image height mismatch");
	Debug::Assert(trainImages.GetWidth() == testImages.GetWidth(), "training / test image width mismatch");

	Debug::Assert(trainImages.GetCount() == trainLabels.GetCount(), "training image count does not equal training label count!");
	Debug::Assert(testImages.GetCount() == testLabels.GetCount(), "test image count does not equal test label count!");
	
	const uint32 imgW = trainImages.GetWidth();
	const uint32 imgH = trainImages.GetHeight();
	const uint32 imgSz = imgW * imgH;
	
	std::cout << CSTR("training: found ", trainImages.GetCount(), " images (", trainImages.GetWidth(), 'x', trainImages.GetHeight(), ")\n");
	std::cout << CSTR("testing: found ", testImages.GetCount(), " images (", trainImages.GetWidth(), 'x', trainImages.GetHeight(), ")\n");
	
	Debug::Assert(imgW == 28 && imgH == 28, CSTR("expected image size of 28x28, instead found (", imgW, 'x', imgH, ")!"));

	std::cout << "Generating input buffers...\n";
	Buffer<Buffer<double>> trainInputs;
	Buffer<Buffer<double>> testInputs;
	trainInputs.SetSize(trainImages.GetCount());
	testInputs.SetSize(testImages.GetCount());

	const size_t maxCount = Maths::Max(trainImages.GetCount(), testImages.GetCount());
	for (size_t i = 0; i < maxCount; ++i)
	{
		if (i < trainInputs.GetSize())
		{
			trainInputs[i].SetSize(imgSz);

			const byte* img = trainImages.GetImage(i);
			for (int p = 0; p < imgSz; ++p)
				trainInputs[i][p] = (double)img[p] / 255.0;
		}

		if (i < testInputs.GetSize())
		{
			testInputs[i].SetSize(imgSz);

			const byte* img = testImages.GetImage(i);
			for (int p = 0; p < imgSz; ++p)
				testInputs[i][p] = (double)img[p] / 255.0;
		}
	}

	Buffer<double> oBuffer;
	oBuffer.SetSize(10);

	Buffer<uint32> batchIndices;
	batchIndices.SetSize(trainInputs.GetSize());

	for (int i = 0; i < batchIndices.GetSize(); ++i)
		batchIndices[i] = i;

	//Debug setup
	Buffer<byte> imgData;
	Texture tex;

	if (debug)
	{
		_previewWindow.SetSize(256, 256);
		_previewWindow.Show();

		_InitTexEnvironment(tex, imgData, imgW, imgH);
	}

	int dotStep = batchIndices.GetSize() / 10;
	WindowEvent e;
	for (int iteration = 0; iteration < iterations; ++iteration)
	{
		std::cout << "ITERATION " << iteration;

		for (int i = 0; i < batchIndices.GetSize(); ++i)
			Utilities::Swap(batchIndices[i], batchIndices[(size_t)(rand.NextFloat() * batchIndices.GetSize())]);

		for (int batchStart = 0; batchStart < batchIndices.GetSize(); batchStart += batchSize)
		{
			_network.BeginTraining();

			for (int batchItem = 0; batchItem < batchSize; ++batchItem)
			{
				const int batchIndex = batchStart + batchItem;
				const int imageIndex = batchIndices[batchIndex];
				Buffer<double>& iBuffer = trainInputs[imageIndex];

				//Train
				if (!_network.Train(iBuffer.Data(), iBuffer.GetSize(), desiredStates[trainLabels.GetLabel(imageIndex)], oBuffer.Data(), 10))
					Debug::PrintLine("TRAINING ERROR: LAYER SIZE MISMATCH!");

				if (batchIndex % dotStep == 0) std::cout << '.';

				if (debug)
				{
					WindowEvent e;
					while (_previewWindow.PollEvent(e))
						if (e.type == WindowEvent::RESIZE)
							glViewport(0, 0, e.data.resize.w, e.data.resize.h);

					for (int i = 0; i < imgData.GetSize(); i += 4)
						imgData[i] = imgData[i + 1] = imgData[i + 2] = iBuffer[i / 4] * 255;

					tex.Modify(0, 0, 0, imgW, imgH, imgData.Data());

					char title[30];
					std::snprintf(title, 30, "%d", trainLabels.GetLabel(imageIndex));
					_previewWindow.SetTitle(title);

					_RenderTextureToWindow(_previewWindow, _program, tex, &_meshes, &_textures);
				}
			}

			_network.ApplyTraining(learningRate);
		}

		if (true)
		{
			std::cout << "| Matched ";

			int matches = 0;
			for (int test = 0; test < testImages.GetCount(); ++test)
			{
				Buffer<double>& iBuffer = testInputs[test];
				_network.Evaluate(iBuffer.Data(), iBuffer.GetSize(), oBuffer.Data(), oBuffer.GetSize());

				int largest = 0;
				for (int i = 1; i < 10; ++i)
					if (oBuffer[i] > oBuffer[largest])
						largest = i;

				if (largest == testLabels.GetLabel(test)) ++matches;
			}

			std::cout << matches << "/" << testImages.GetCount() << "\n";
		}
	}

	if (debug) _previewWindow.Hide();

	Buffer<byte> outBuffer;
	ByteWriter outWriter(outBuffer);
	_network.Write(outWriter);
	IO::WriteFile("Data/net-state.bin", outBuffer);
}

void Digits::Draw()
{
	if (!_ReadNetStateFromFile(_network)) return;
	
	_previewWindow.SetSize(256, 256);
	_previewWindow.Show();

	Texture tex;
	Buffer<byte> imgData;
	const int w = Maths::SquareRoot(_network.InputLayer().GetSize());
	_InitTexEnvironment(tex, imgData, w, w);

	Buffer<double> iBuffer;
	iBuffer.SetSize(_network.InputLayer().GetSize());

	double oBuffer[10];

	float penRadiusSq = 5.f;
	bool drawing = false;
	while (true)
	{
		DrawingUpdate du = _HandleDrawingWindow(_previewWindow, tex, imgData, drawing, penRadiusSq);
		if ((int)(du & DrawingUpdate::EXIT))
			break;

		if ((int)(du & DrawingUpdate::STROKE))
		{
			//Evaluate network
			for (size_t i = 0; i < iBuffer.GetSize(); ++i)
				iBuffer[i] = imgData[i * 4] / 255.0;

			std::cout << "Evaluating... ";
			if (_network.Evaluate(iBuffer.Data(), iBuffer.GetSize(), oBuffer, 10))
			{
				int largest = 0;
				for (int i = 1; i < 10; ++i)
					if (oBuffer[i] > oBuffer[largest])
						largest = i;

				std::cout << largest;

				for (int i = 0; i < 10; ++i)
					std::cout << " [" << (int)(oBuffer[i] * 100) << "%]";

				std::cout << '\n';
			}
			else std::cout << "(ERROR)\n";
		}

		tex.Modify(0, 0, 0, w, w, imgData.Data());

		_RenderTextureToWindow(_previewWindow, _program, tex, &_meshes, &_textures);
	}
}

void Digits::MTrain()
{
	std::cout << "Loading training data...\n";
	Buffer<byte> trainImageData = IO::ReadFile("Data/train-images.idx3-ubyte");
	Buffer<byte> trainLabelData = IO::ReadFile("Data/train-labels.idx1-ubyte");

	ImagesIDX3 trainImages;
	LabelsIDX1 trainLabels;

	{
		ByteReader trainImageReader(trainImageData);
		if (!trainImages.Read(trainImageReader)) return;

		ByteReader trainLabelReader(trainLabelData);
		if (!trainLabels.Read(trainLabelReader)) return;
	}

	_previewWindow.SetSize(256, 256);
	_previewWindow.Show();

	Texture tex;
	Buffer<byte> imgData;
	Buffer<byte> trainData;
	_InitTexEnvironment(tex, imgData, trainImages.GetWidth(), trainImages.GetHeight());
	trainData.SetSize((size_t)trainImages.GetWidth() * trainImages.GetHeight());

	Random rand(Time::GetRandSeed());

	int di = 11; //reset
	int digits[10];
	for (int i = 0; i < 10; ++i)
		digits[i] = i;

	bool active = true;
	bool drawing = false;
	while (active)
	{
		if (di > 9)
		{
			//reshuffle
			for (int i = 0; i < 10; ++i)
				Utilities::Swap(digits[i], digits[(int)(rand.NextFloat() * 10)]);

			di = 0;
		}

		const int digit = digits[di++];
		_previewWindow.SetTitle(CSTR("Draw ", digit));

		while (true)
		{
			DrawingUpdate du = _HandleDrawingWindow(_previewWindow, tex, imgData, drawing, 5.f);

			if ((int)(du & DrawingUpdate::EXIT))
			{
				active = false;
				break;
			}

			if ((int)(du & DrawingUpdate::SUBMIT))
			{
				for (int i = 0; i < trainData.GetSize(); ++i)
					trainData[i] = imgData[i * 4];

				trainImages.AddImage(trainData);
				trainLabels.AddLabel(digit);

				std::cout << "Added " << digit << '\n';

				//clear
				for (int p = 0; p < imgData.GetSize(); p += 4)
					imgData[p] = imgData[p + 1] = imgData[p + 2] = 0;

				break;
			}

			tex.Modify(0, 0, 0, tex.GetWidth(), tex.GetHeight(), imgData.Data());
			_RenderTextureToWindow(_previewWindow, _program, tex, &_meshes, &_textures);
		}
	}

	std::cout << "Done\nWriting\nSet size is now " << trainImages.GetCount() << "\nWriting...\n";

	ByteWriter trainImageWriter(trainImageData);
	trainImages.Write(trainImageWriter);
	IO::WriteFile("Data/train-images.idx3-ubyte", trainImageData);

	ByteWriter trainLabelWriter(trainLabelData);
	trainLabels.Write(trainLabelWriter);
	IO::WriteFile("Data/train-labels.idx1-ubyte", trainLabelData);
}

int Digits::Run()
{
	_ctx.CreateDummyAndUse();
	GL::LoadDummyExtensions();

	_previewWindow.Create("Preview", nullptr, WindowFlags::OPENGL);
	_previewWindow.SetDestroyOnClose(false);

	_ctx.Create(_previewWindow);
	_ctx.Use(_previewWindow);
	GL::LoadExtensions(_previewWindow);

	wglSwapIntervalEXT(0);

	_program.Load("Data/Shaders/Unlit.vert", "Data/Shaders/Unlit.frag");

	_meshes.Initialise();
	_textures.Initialise();

	bool active = true;
	while (active)
	{
		String cmd;
		std::cout << "DIGITS>";
		std::getline(std::cin, cmd);

		Buffer<String> tokens = cmd.Split(" \r\n");
		if (tokens.GetSize() > 0)
		{
			String first = tokens[0].ToLower();

			if (first == "help")
			{
				std::cout <<
					"DIGIT RECOGNISER\n"
					"----------------\n"
					"gen [iterations=10] [batch_size=10] [layer_size=30] [learning_rate=3] [debug=0]\t\tgenerate a new network\n"
					"train [iterations=10] [batch_size=10] [learning_rate=3] [debug=0]\t\t\ttrain existing network\n"
					"draw\t\t\t\t\t\t\t\t\t\t\tdraw digits yourself\n"
					"mtrain\t\t\t\t\t\t\t\t\t\t\tappend new training images\n"
					"exit\t\t\t\t\t\t\t\t\t\t\t...\n";
			}
			else if (first == "gen")
			{
				int iterations = 10;
				int batchSize = 10;
				int layerSize = 30;
				double learningRate = 3.0;
				bool debug = false;

				if (tokens.GetSize() > 1)
					iterations = tokens[1].ToInt();
				if (tokens.GetSize() > 2)
					batchSize = tokens[2].ToInt();
				if (tokens.GetSize() > 3)
					layerSize = tokens[3].ToInt();
				if (tokens.GetSize() > 4)
					learningRate = tokens[4].ToFloat();
				if (tokens.GetSize() > 5)
					debug = tokens[5].ToInt() != 0;

				Train(iterations, batchSize, layerSize, learningRate, debug);
			}
			else if (first == "train")
			{
				int iterations = 10;
				int batchSize = 10;
				double learningRate = 3.0;
				bool debug = false;

				if (tokens.GetSize() > 1)
					iterations = tokens[1].ToInt();
				if (tokens.GetSize() > 2)
					batchSize = tokens[2].ToInt();
				if (tokens.GetSize() > 3)
					learningRate = tokens[3].ToFloat();
				if (tokens.GetSize() > 4)
					debug = tokens[4].ToInt() != 0;

				Train(iterations, batchSize, -1, learningRate, debug);
			}
			else if (first == "mtrain")
			{
				MTrain();
			}
			else if (first == "draw")
			{
				Draw();
			}
		}
	}

	return 0;
}
