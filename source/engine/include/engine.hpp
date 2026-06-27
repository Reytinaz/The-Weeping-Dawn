#ifndef ENGINE_H
#define ENGINE_H

#include "physics.hpp"
#include "sceneManager.hpp"
#include "renderer.hpp"
#include "worldManager.hpp"
#include "dimensionManager.hpp"
#include "functional"
#include "thread"
#include "map"

#ifdef _WIN32
	#include "windows.h"
	#include "commctrl.h"
	#include "psapi.h"
#endif

struct keyBindData {
	sf::Keyboard::Key key;
	std::function<void(Engine&)> func;
};
struct userSettings {
	float mouse_sensivity = 0.4f;
	float view_distance = 0.75f;
	bool invert_y = false;
	float gamma = 1.0f;
	float texuresSize = 0.4f;
	bool showHieararchy = false;
	bool freeCameraEnabled = false;
};

class Engine {
private:
	// window content:

	sf::RenderWindow* window;
	sf::VideoMode vidMode;
	sf::ContextSettings contSettings;
	sf::Vector2i mousePos;
	sf::Vector2f mouseWorldPos;
	Renderer* renderer;

	#ifdef _WIN32
		static LRESULT CALLBACK SubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
		static const UINT_PTR SubclassId = 1;
	#endif

	bool clickedOnText = false;
	bool canClick = true;
	bool inWindow = false;
	bool mouseUnlocked = false;
	bool debugEnabled = false;

	std::unordered_map<Instance*, bool> expanded;

	// application variables:

	std::vector<keyBindData> keyBinds;
	std::shared_ptr<Character> character;
	std::vector<std::thread> threads;
	sf::Clock clock;
	float deltaTime;
	int currentSeed = 12345;
	std::unordered_map<std::string, std::shared_ptr<Object3D>> modelCache;


	// private funtioncs:

	void initVariables();
	void initWindow();
	void initOpenGL() const;
public:
	Engine();
	~Engine();

	std::string username;
	userSettings usrSettings;
	SceneManager scene;
	WorldManager worldManagment;
	std::map<std::string, std::shared_ptr<Dimension>> dimensions;
	std::shared_ptr<Dimension> currentDimension;
	std::string currentWorldPath = "";
	std::vector<std::shared_ptr<Object3D>> globalIgnoreList;
	std::string selectedWorld = "";

	bool inMenu = false;
	bool paused = false;

	void update();
	void render();
	void logic();

	std::filesystem::path getExecutablePath();

	void bindKey(const sf::Keyboard::Key&, const std::function<void(Engine& engine)>& func);
	void unbindKey(const sf::Keyboard::Key&);

	sf::Font loadFont(const std::string&);
	sf::Texture createTexture(const std::string&);
	void createThread(const std::function<void(Engine& engine)>& func);
	void setCharacter(std::shared_ptr<Character> character);
	void setCharacter();
	void setMouseCursorGrabbed(bool value);
	void setMouseUnlocked(bool value);
	void setMouseCursorVisible(bool value); 
	void pushObj3D(std::shared_ptr<Object3D> obj);
	void renderHierarchy();
	bool loadWorld(const std::string& filename);
	void clearWorld();
	void clearDimension(const Dimension& dimension);
	void loadDimension(const std::string& name, unsigned int seed, std::shared_ptr<PerlinNoise> perlinNoise, ChunkManager& cm);
	void spawnCharacter();
	void saveToChunk(std::shared_ptr<Instance> obj);
	std::map<std::string, std::string> getWorldsList() const;
	std::shared_ptr<Object3D> getModel(const std::string& path);

	size_t getAvailableStackSpace();
	size_t getMemoryUsageBytes() const;
	const float getDeltaTime() const;
	std::shared_ptr<Camera>& getMainCamera();
	Renderer& getRenderer();
	const bool isRunning() const;
	const bool isDebuging() const;
	float degToRad(float degrees);
	float radToDeg(float radians);
	void setDebugMode(bool b);
	void exit();
};

#endif