#pragma once
#include "LayeredNetwork.hpp"
#include <ELGraphics/MeshManager.hpp>
#include <ELGraphics/TextureManager.hpp>
#include <ELSys/GLContext.hpp>
#include <ELSys/GLProgram.hpp>
#include <ELSys/Window.hpp>

class Digits
{
	LayeredNetwork _network;

	//
	Window _previewWindow;
	GLContext _ctx;
	GLProgram _program;
	MeshManager _meshes;
	TextureManager _textures;

public:
	//if LayerSize is less than 0 the network will be read from file
	void Train(int iterations, int batchSize, int layerSize, double learningRate, bool debug);
	
	void Draw();

	void MTrain();

	int Run();
};
