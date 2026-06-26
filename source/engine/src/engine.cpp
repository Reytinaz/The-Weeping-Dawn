#include "targetArch.hpp"
#include "engine.hpp"
#include "glad.h"
#include "basetsd.h"
#include "processthreadsapi.h"
#include "sstream"

namespace {
	std::filesystem::path resourcesDir() {
		return "";
	}
}

LRESULT CALLBACK Engine::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR subclassId, DWORD_PTR refData) {
	Engine* engine = (Engine*)refData;
	switch (msg) {
	case WM_SYSCOMMAND:
		if (wParam == SC_RESTORE || wParam == SC_MINIMIZE)
			return 0;
		break;
	case WM_NCLBUTTONDOWN:
		if (wParam == HTCAPTION)
			return 0;
		break;
	case WM_SIZING:
		return TRUE;
	case WM_CLOSE:
		engine->window->close();
		return 0;
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void renderBoundingBox(const std::shared_ptr<Object3D>& obj, const Camera& camera) {
	if (!obj) return;

	Matrix4 view = camera.getViewMatrix();
	Matrix4 proj = camera.getProjectionMatrix();
	Matrix4 transform = obj->getTransform();

	Vector3 localCorners[8] = {
		Vector3(-0.5f, -0.5f, -0.5f),
		Vector3(0.5f, -0.5f, -0.5f),
		Vector3(0.5f, -0.5f,  0.5f),
		Vector3(-0.5f, -0.5f,  0.5f),
		Vector3(-0.5f,  0.5f, -0.5f),
		Vector3(0.5f,  0.5f, -0.5f),
		Vector3(0.5f,  0.5f,  0.5f),
		Vector3(-0.5f,  0.5f,  0.5f)
	};

	Vector3 worldCorners[8];
	for (int i = 0; i < 8; ++i) {
		worldCorners[i] = transform * localCorners[i];
	}

	int edges[12][2] = {
		{0,1}, {1,2}, {2,3}, {3,0},
		{4,5}, {5,6}, {6,7}, {7,4},
		{0,4}, {1,5}, {2,6}, {3,7}
	};

	GLboolean depthTestEnabled;
	glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
	GLint prevProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	glUseProgram(0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(proj.ptr());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(view.ptr());

	glDisable(GL_DEPTH_TEST);

	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	for (auto& edge : edges) {
		glVertex3f(worldCorners[edge[0]].x, worldCorners[edge[0]].y, worldCorners[edge[0]].z);
		glVertex3f(worldCorners[edge[1]].x, worldCorners[edge[1]].y, worldCorners[edge[1]].z);
	}
	glEnd();

	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(1.0f, 1.0f, 0.0f);

	Vector3 centerFromMatrix = transform * Vector3(0, 0, 0);
	glVertex3f(centerFromMatrix.x, centerFromMatrix.y, centerFromMatrix.z);
	glEnd();

	if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	if (prevProgram) glUseProgram(prevProgram);
}

void Engine::initVariables() {
	this->window = nullptr;
	this->contSettings.depthBits = 24;
	this->contSettings.sRgbCapable = false;
	this->contSettings.stencilBits = 8;
	this->contSettings.antiAliasingLevel = 2;
	this->contSettings.majorVersion = 3;
	this->contSettings.minorVersion = 3;
	this->usrSettings = userSettings();
	this->renderer = new Renderer(vidMode.getDesktopMode().size.x, vidMode.getDesktopMode().size.y);
	this->renderer->camera = std::make_shared<Camera>();
}

void Engine::initWindow() {
	RECT workArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	sf::Vector2u winSize(workArea.right - workArea.left, workArea.bottom - workArea.top);
	sf::Vector2i winPos(workArea.left, workArea.top);

	window = new sf::RenderWindow(sf::VideoMode(winSize), "The Weeping Dawn",
		sf::Style::Default,
		sf::State::Windowed,
		contSettings);
	window->setFramerateLimit(60);
	window->setVerticalSyncEnabled(true);
	if (!window->setActive(true))
		std::cout << "Failed to activate the Window" << std::endl;

	#ifdef _WIN32
		HWND hwnd = window->getNativeHandle();
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hwnd, GWL_STYLE, style);

		SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

		HMENU sysMenu = GetSystemMenu(hwnd, FALSE);
		RemoveMenu(sysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
		SetWindowSubclass(hwnd, SubclassProc, SubclassId, (DWORD_PTR)this);

		int clientW = workArea.right - workArea.left;
		int clientH = workArea.bottom - workArea.top;

		RECT rc = { 0, 0, clientW, clientH };
		AdjustWindowRect(&rc, style, FALSE);
		int fullW = rc.right - rc.left;
		int fullH = rc.bottom - rc.top;
		int posX = workArea.left + rc.left;
		int posY = workArea.top + rc.top;

		SetWindowPos(hwnd, HWND_TOP, posX, posY, fullW, fullH, SWP_NOZORDER);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		window->setSize(sf::Vector2u(clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top));
	#endif
}

void Engine::initOpenGL() const {
	std::cout << "=== GLAD INIT ===" << std::endl;
	if (!window->setActive(true)) {
		std::cout << "Failed to activate OpenGL context!" << std::endl;
	}
	if (!gladLoadGLLoader((GLADloadproc)sf::Context::getFunction)) {
		std::cout << "GLAD initialization FAILED!" << std::endl;
		return;
	}
	std::cout << "GLAD initialized successfully" << std::endl;
	const char* version = (const char*)glGetString(GL_VERSION);
	if (version) {
		std::cout << "OpenGL Version: " << version << std::endl;
	}
	else {
		std::cout << "OpenGL Version: NULL (glGetString failed)" << std::endl;
	}
	if (!renderer->init()) {
		std::cerr << "Failed to initialize renderer" << std::endl;
	}
}

Engine::Engine() {
	initVariables();
	initWindow();
	initOpenGL();
}

Engine::~Engine() {
	for (auto& t : threads) {
		if (t.joinable()) t.join();
	}
	if (window) {
		HWND hwnd = window->getNativeHandle();
		RemoveWindowSubclass(hwnd, SubclassProc, SubclassId);
		delete window;
	}
	delete renderer;
}

void Engine::update() {
	mousePos = sf::Mouse::getPosition(*window);
	mouseWorldPos = window->mapPixelToCoords(mousePos);
	deltaTime = clock.restart().asSeconds();
	inWindow = window->hasFocus();
	if (currentDimension && currentDimension->chunkManager && currentDimension->physics) {
		currentDimension->chunkManager->update(renderer->camera->position, usrSettings.view_distance);
		currentDimension->physics->update(deltaTime);
	}
	if (character) {
		character->update(deltaTime);
		inMenu = false;
	} else inMenu = true;
	if (renderer->camera) renderer->camera->updateVectorsFromView();
	renderer->setDebug(debugEnabled);
}

void Engine::render() {
	window->clear();
	renderer->clear();
	auto objects = scene.collect3DObjects();
	auto lights = scene.collectLights();
	renderer->render(objects, lights, usrSettings.view_distance, usrSettings.gamma);
	if (currentDimension) currentDimension->renderSkybox(renderer->camera);
	if (debugEnabled) {
		for (auto& obj : objects) {
			renderBoundingBox(obj, *renderer->camera);
		}
	}
	window->pushGLStates();
	window->resetGLStates();
	scene.renderInterface(*window);
	if (debugEnabled && usrSettings.showHieararchy) renderHierarchy();
	window->popGLStates();
	window->display();
}

void Engine::logic() {
	while (const std::optional event = window->pollEvent()) {
		if (event->is<sf::Event::Closed>()) {
			window->close();
		}
		if (!paused) {
			if (renderer->camera->cameraType == CameraType::FIRST_PERSON) {
				if (inWindow && character) {
					if (!mouseUnlocked) {
						const sf::Vector2i& mousePos = sf::Mouse::getPosition();
						const sf::Vector2i center(window->getSize().x / 2, window->getSize().y / 2);
						const sf::Vector2i& delta = mousePos - center;

						float sensitivity = usrSettings.mouse_sensivity * 0.025f;
						const float maxPitch = 1.5f;
						renderer->camera->rotation.x += delta.y * sensitivity * (usrSettings.invert_y ? -1 : 1);
						renderer->camera->rotation.x = std::max(-maxPitch, std::min(maxPitch, renderer->camera->rotation.x));
						renderer->camera->rotation.y -= delta.x * sensitivity;

						sf::Mouse::setPosition(center, *this->window);
					}
					renderer->camera->position = character->position + character->cameraOffset;
					character->rotation.y = renderer->camera->rotation.y;
				}
				if (this->inWindow && this->character) {
					if (event->is<sf::Event::KeyPressed>()) {
						switch (event->getIf<sf::Event::KeyPressed>()->code) {
						case sf::Keyboard::Key::W: character->handleInput(CharacterInput::FORWARD, false); break;
						case sf::Keyboard::Key::S: character->handleInput(CharacterInput::BACKWARD, false); break;
						case sf::Keyboard::Key::A:character->handleInput(CharacterInput::LEFT, false); break;
						case sf::Keyboard::Key::D: character->handleInput(CharacterInput::RIGHT, false); break;
						case sf::Keyboard::Key::Space: character->handleInput(CharacterInput::JUMP, false); break;
						case sf::Keyboard::Key::LShift: character->handleInput(CharacterInput::RUN, false); break;
						case sf::Keyboard::Key::LControl: character->handleInput(CharacterInput::CROUCH, false); break;
						}
					}
					if (event->is<sf::Event::KeyReleased>()) {
						switch (event->getIf<sf::Event::KeyReleased>()->code) {
						case sf::Keyboard::Key::W: character->handleInput(CharacterInput::FORWARD, true); break;
						case sf::Keyboard::Key::S: character->handleInput(CharacterInput::BACKWARD, true); break;
						case sf::Keyboard::Key::A:character->handleInput(CharacterInput::LEFT, true); break;
						case sf::Keyboard::Key::D: character->handleInput(CharacterInput::RIGHT, true); break;
						case sf::Keyboard::Key::Space: character->handleInput(CharacterInput::JUMP, true); break;
						case sf::Keyboard::Key::LShift: character->handleInput(CharacterInput::RUN, true); break;
						case sf::Keyboard::Key::LControl: character->handleInput(CharacterInput::CROUCH, true); break;
						}
					}
				}
			}
			
			else if (renderer->camera->cameraType == CameraType::FREECAM) {
				character->handleInput(CharacterInput::FORWARD, true);
				character->handleInput(CharacterInput::BACKWARD, true);
				character->handleInput(CharacterInput::LEFT, true);
				character->handleInput(CharacterInput::RIGHT, true);
				character->handleInput(CharacterInput::JUMP, true);
				character->handleInput(CharacterInput::RUN, true);
				character->handleInput(CharacterInput::CROUCH, true);

				if (!mouseUnlocked) {
					const sf::Vector2i& mousePos = sf::Mouse::getPosition();
					const sf::Vector2i center(window->getSize().x / 2, window->getSize().y / 2);
					const sf::Vector2i& delta = mousePos - center;
					float sensitivity = usrSettings.mouse_sensivity * 0.01f;
					const float maxPitch = 1.5f;
					renderer->camera->rotation.x += delta.y * sensitivity * (usrSettings.invert_y ? -1 : 1);
					renderer->camera->rotation.x = std::max(-maxPitch, std::min(maxPitch, renderer->camera->rotation.x));
					renderer->camera->rotation.y -= delta.x * sensitivity;
					sf::Mouse::setPosition(center, *this->window);
				}

				float speed = 5.0f * deltaTime;
				Vector3 moveDir(0, 0, 0);
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) moveDir += renderer->camera->forward();
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) moveDir -= renderer->camera->forward();
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) moveDir += renderer->camera->right();
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) moveDir -= renderer->camera->right();
				moveDir.y = 0;
				if (moveDir.length() > 0) {
					moveDir = moveDir.normalized() * speed;
				}
				renderer->camera->position = renderer->camera->position + moveDir;
			}
		}

		if (event->is<sf::Event::KeyReleased>()) {
			for (auto& e : keyBinds) {
				if (event->getIf<sf::Event::KeyReleased>()->code == e.key) e.func(*this);
			}
		}

		if (event->is<sf::Event::MouseButtonReleased>()) {
			sf::Vector2f mousePos = window->mapPixelToCoords(sf::Mouse::getPosition(*window));
			std::function<void(std::shared_ptr<Instance>)> traverse = [&](std::shared_ptr<Instance> node) {
				if (auto btn = std::dynamic_pointer_cast<Button>(node)) {
					if (btn->isVisible() && btn->contains(mousePos, *window)) {
						btn->onClick(*this, btn);
					}
				}
				if (!node->children.empty()) {
					for (auto& child : node->children) {
						if (child) {
							traverse(child);
						}
					}
				}
			};
			traverse(scene.interface);
		}
		std::function<void(std::shared_ptr<Instance>)> traverse = [&](std::shared_ptr<Instance> node) {
			if (auto btn = std::dynamic_pointer_cast<Button>(node)) {
				if (btn->isVisible() && btn->contains(sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)), *window)) {
					if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
						btn->shape->setFillColor(btn->pressedColor);
					}
					else {
						btn->shape->setFillColor(btn->hoverColor);
					}
				}
				else {
					btn->shape->setFillColor(btn->normalColor);
				}
			}
			for (auto& child : node->children) traverse(child);
		};
		traverse(scene.interface);
		/*
		// Text Box 2
		if (this->clickedOnText) {
			if (event->is<sf::Event::TextEntered>()) {
				if (event->getIf<sf::Event::TextEntered>()->unicode < 128) {
					if (event->getIf<sf::Event::TextEntered>()->unicode == 8) { // Backspace
						if (!this->selectedText->text.getString().isEmpty()) {
							this->selectedText->text.setString(this->selectedText->text.getString().substring(0, this->selectedText->text.getString().getSize() - 1));
						}
					}
					else if (event->getIf<sf::Event::TextEntered>()->unicode < 128 && event->getIf<sf::Event::TextEntered>()->unicode != 13) {
						this->selectedText->text.setString(this->selectedText->text.getString() + static_cast<char>(event->getIf<sf::Event::TextEntered>()->unicode));
					}
				}
			}
		}
		*/
	}
}

void Engine::createThread(const std::function<void(Engine& engine)>& func) {
	//std::thread(func, std::ref(*this)).detach();
	threads.emplace_back([this, func] { func(*this); });
}

void Engine::bindKey(const sf::Keyboard::Key& key, const std::function<void(Engine& engine)>& func) {
	keyBinds.push_back({ key, func });
}

void Engine::unbindKey(const sf::Keyboard::Key& key) {
	keyBinds.erase(
		remove_if(keyBinds.begin(), keyBinds.end(),
			[&](const keyBindData& s) { return s.key == key; }),
		keyBinds.end());
}

sf::Font Engine::loadFont(const std::string& path) {
	return sf::Font(resourcesDir() / path);
}

sf::Texture Engine::createTexture(const std::string& path) {
	return sf::Texture(resourcesDir() / path);
}
void Engine::setCharacter(std::shared_ptr<Character> char1) {
	character = char1;
}
void Engine::setCharacter() {
	character.reset();
}

void Engine::setMouseCursorGrabbed(bool value) {
	window->setMouseCursorGrabbed(value);
}

void Engine::setMouseUnlocked(bool value) {
	mouseUnlocked = value;
}

void Engine::setMouseCursorVisible(bool value) {
	window->setMouseCursorVisible(value);
}

void Engine::spawnCharacter() {
	if (currentDimension) {
		auto player = scene.workspace->addChild<Character>("Player");
		player->position = currentDimension->spawnPoint;
		player->scale = Vector3(0.5f, 1.25f, 0.5f);
		player->m_params.walkSpeed = 4.0f;
		player->m_params.runSpeed = 6.0f;
		player->m_params.jumpForce = 9.0f;
		player->restitution = 0.1f;
		player->friction = 0.1f;
		player->visible = false;
		player->useGravity = true;
		player->isStatic = false;
		player->canRaycast = false;
		player->mass = 1.0f;
		player->m_camera = renderer->camera;
		setCharacter(player);
		pushObj3D(player);
	}
}

void Engine::saveToChunk(std::shared_ptr<Instance> obj) {
	if (currentDimension->chunkManager) {
		if (auto obj3d = std::dynamic_pointer_cast<Object3D>(obj)) {
			auto chunk = currentDimension->chunkManager->getChunkAt(obj3d->position.x, obj3d->position.z);
			if (chunk) {
				chunk->objects.push_back(obj);
				worldManagment.saveChunk(currentWorldPath, currentDimension->name, chunk);
			}
		}
		if (auto srcLight = std::dynamic_pointer_cast<SourceLight>(obj)) {
			auto chunk = currentDimension->chunkManager->getChunkAt(srcLight->position.x, srcLight->position.z);
			if (chunk) {
				chunk->objects.push_back(obj);
				worldManagment.saveChunk(currentWorldPath, currentDimension->name, chunk);
			}
		}
	}
}

void Engine::pushObj3D(std::shared_ptr<Object3D> obj) {
	obj->setupBuffers();
	obj->initOpenGL();
	if (currentDimension) {
		currentDimension->physics->addObject(obj);
		auto ch = std::dynamic_pointer_cast<Character>(obj);
		if (ch) {
			ch->physics = currentDimension->physics;
			ch->chunkManager = currentDimension->chunkManager;
		}
		if (!ch) {
			saveToChunk(obj);
		}
	}
	obj->applyModelScale();
}

void Engine::renderHierarchy() {
	float startX = 1700.0f;
	float startY = 10.0f;
	const float lineHeight = 22.0f;
	const float indent = 20.0f;

	auto font = loadFont("assets/fonts/tuffy.ttf");
	auto backgrounnd = sf::RectangleShape(sf::Vector2f(250.0f, (float)(this->vidMode.size.y)));
	sf::Text title(font, L"Hierarchy", 20);
	title.setFillColor(sf::Color::Yellow);
	title.setPosition(sf::Vector2f(startX, startY));
	title.setOutlineThickness(1.f);
	backgrounnd.setPosition(sf::Vector2f(startX - 10.0f, 0));
	window->draw(title);
	window->draw(backgrounnd);
	startY += lineHeight + 5;

	std::function<void(std::shared_ptr<Instance>, int depth)> drawNode =
		[&](std::shared_ptr<Instance> node, int depth) {
		std::stringstream ss;

		for (int i = 0; i < depth; ++i) ss << "  ";
		ss << ("[ ] ") << node->name
			<< " (" << node->className << ")";

		sf::Text text(font, ss.str(), 18);
		text.setFillColor(sf::Color(50, 50, 50));
		text.setPosition(sf::Vector2f(startX, startY));
		startY += lineHeight;
		window->draw(text);

		for (auto& child : node->children) {
			drawNode(child, depth + 1);
		}
	};
	drawNode(scene.interface, 0);
	drawNode(scene.workspace, 0);
}

void Engine::loadDimension(const std::string& name, unsigned int seed, std::shared_ptr<PerlinNoise> perlinNoise, ChunkManager& cm) {
	auto it = dimensions.find(name);
	if (it == dimensions.end()) {
		std::cerr << "Dimension '" << name << "' not found!" << std::endl;
		return;
	}
	clearWorld();

	auto& dimension = it->second;

	dimension->chunkManager = &cm;
	dimension->physics = new Physics;
	dimension->skybox = std::make_shared<Skybox>();
	dimension->skybox->thisDim = dimension;

	dimension->chunkManager->engine = this;
	dimension->chunkManager->thisDim = dimension;
	dimension->chunkManager->seed = seed;
	dimension->chunkManager->noise = perlinNoise;
	dimension->chunkManager->setNoiseParams(
		dimension->noiseParams[0],
		static_cast<int>(dimension->noiseParams[1]),
		dimension->noiseParams[2]
	);
	dimension->physics->setChunkManager(dimension->chunkManager);

	if (dimension->skybox) {
		dimension->skybox->renderer = renderer;
		dimension->skybox->load(dimension->skyboxFaces, dimension->celestialBodies);
	}

	renderer->setChunkManager(dimension->chunkManager);
	renderer->setCurrentDimension(&*dimension);
	currentDimension = dimension;
	/*
	auto sunlight = scene.workspace->addChild<SourceLight>("Sunlight");
	sunlight->type = dimension->sunlight->type;
	sunlight->color = dimension->sunlight->color;
	sunlight->enabled = dimension->sunlight->enabled;
	sunlight->position = dimension->sunlight->position;
	sunlight->direction = dimension->sunlight->direction;
	sunlight->intensity = dimension->sunlight->intensity;
	sunlight->constant = dimension->sunlight->constant;
	sunlight->linear = dimension->sunlight->linear;
	sunlight->cutOff = dimension->sunlight->cutOff;
	sunlight->outerCutOff = dimension->sunlight->outerCutOff;
	sunlight->quadratic = dimension->sunlight->quadratic;
	dimension->sunlight = sunlight;
	*/
}

bool Engine::loadWorld(const std::string& filename) {
	dimensions.clear();

	int seed = worldManagment.getWorldSeed(filename);
	auto perlinNoise = std::make_shared<PerlinNoise>(seed);
	auto chunkManager = new ChunkManager(32, 1.0f, 20.0f, 0.5);
	std::map<std::string, std::shared_ptr<Dimension>> loadedDims;

	if (!worldManagment.load(filename, loadedDims, perlinNoise, *chunkManager)) {
		std::cerr << "Failed to load world: " << filename << std::endl;
		return false;
	}

	dimensions = loadedDims;
	currentWorldPath = filename;
	currentSeed = seed;

	if (!dimensions.empty()) {
		loadDimension(dimensions.begin()->first, seed, perlinNoise, *chunkManager);
	}

	std::cout << "World loaded successfuly!" << std::endl;

	return true;
}


void Engine::clearWorld() {
	scene.workspace->removeAll();
	if (currentDimension) currentDimension->physics->clear();
}

void Engine::clearDimension(const Dimension& dimension) {
	dimension.physics->clear();
}

std::map<std::string, std::string> Engine::getWorldsList() const {
	namespace fs = std::filesystem;
	std::map<std::string, std::string> files;
	fs::create_directory("assets/data/worlds");
	for (const auto& entry : fs::directory_iterator("assets/data/worlds")) {
		if (entry.is_regular_file() && entry.path().extension() == ".json") {
			files.insert({ entry.path().stem().string(), entry.path().string()});
		}
	}
	return files;
}
std::shared_ptr<Object3D> Engine::getModel(const std::string& path) {
	auto it = modelCache.find(path);
	if (it != modelCache.end()) {
		return it->second;
	}
	auto model = std::make_shared<Object3D>("model", ObjectType::CUSTOM);
	if (model->loadOBJ(path)) {
		modelCache[path] = model;
		return model;
	}
	return nullptr;
}
size_t Engine::getAvailableStackSpace() {
	ULONG_PTR lowLimit, highLimit;
	GetCurrentThreadStackLimits(&lowLimit, &highLimit);
	int stackVar;
	ULONG_PTR currentStackPtr = reinterpret_cast<ULONG_PTR>(&stackVar);

	size_t remainingSpace = currentStackPtr - lowLimit;

	if (currentStackPtr < lowLimit || currentStackPtr > highLimit) {
		std::cout << "Warning: Stack pointer is outside the expected limits" << std::endl;
	}

	return remainingSpace;
}
size_t Engine::getMemoryUsageBytes() const {
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		return pmc.WorkingSetSize;   // физическая память процесса (байт)
	}
	return 0;
}
const float Engine::getDeltaTime() const {
	return deltaTime;
}
std::shared_ptr<Camera>& Engine::getMainCamera() {
	return renderer->camera;
}
Renderer& Engine::getRenderer() {
	return *renderer;
}
void Engine::exit() {
	window->close();
}
const bool Engine::isRunning() const {
	return window->isOpen();
}
const bool Engine::isDebuging() const {
	return debugEnabled;
}
float Engine::degToRad(float degrees) {
	return static_cast<float>(degrees * M_PI / 180.0f);
}
float Engine::radToDeg(float radians) {
	return static_cast<float>(radians * 180.0f / M_PI);
}
void Engine::setDebugMode(bool b) {
	debugEnabled = b;
}