#pragma once
#include "UIConnection.hpp"
#include "UINode.hpp"
#include <ELCore/Context.hpp>
#include <ELCore/Tracker.hpp>
#include <ELGraphics/FontManager.hpp>
#include <ELGraphics/MaterialManager.hpp>
#include <ELGraphics/Material_Sprite.hpp>
#include <ELGraphics/MeshManager.hpp>
#include <ELGraphics/RenderQueue.hpp>
#include <ELGraphics/TextureManager.hpp>
#include <ELMaths/Projection.hpp>
#include <ELMaths/Transform.hpp>
#include <ELSys/GLContext.hpp>
#include <ELSys/GLProgram.hpp>
#include <ELSys/InputManager.hpp>
#include <ELSys/Window.hpp>
#include <ELUI/Container.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

class Sandbox
{
private:
	Context _context;
	FT_Library _ftLib;
	FontManager _fontManager;
	MaterialManager _materialManager;
	MeshManager _meshManager;
	TextureManager _textureManager;
	Window _window;

	GLContext _gl;
	GLProgram _unlit;

	UIContainer _ui;
	RenderQueue _uiQueue;
	MaterialSprite _uiNodeMat;
	SharedPointer<const Material> _uiNodeMatPtr;
	SharedPointer<const Font> _uiFont;
	Tracker<UINode> _uiNodeTracker;

	List<NetPerceptron> _perceptrons;
	Tracker<NetPerceptron> _pTracker;
	Hashmap<NetPerceptron*, UINode*> _p2n;

	struct Camera
	{
		Projection projection;
		Transform transform;
	} _camera;

	float _lastFrameTime;

	float _mX;
	float _mY;

	struct _Connnection
	{
		SharedPointer<UINode> start;

	} _connection;

	void _Init();
	void _Cleanup();

	bool _Loop();
	void _Frame();
	void _Render();

	void _MouseUp();
	void _MouseDown();
	void _MouseMove(uint16 x, uint16 y);

	void _KeyUp(EKeycode);
	void _KeyDown(EKeycode);

	UINode* _CreateNode(float x, float y);
	void _DeleteNode(UINode& node);

	void _StartConnection(UINode& from);
	void _EndConnection(UINode& to);
	void _DeleteConnection(UIConnection&);

	void _UpdateNet();
public:
	int Run();
};

