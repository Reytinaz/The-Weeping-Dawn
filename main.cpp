#include "targetArch.hpp"
#include "engine.hpp"

using namespace std;
static bool mouseUnlocked = false;

static void updateWorldsList(Engine& engine, sf::Font& globalFont) {
	engine.scene.interface->findChild("MenuUI")->visible = false;
	engine.scene.interface->findChild("WorldsUI")->visible = true;
	engine.scene.interface->findChild("WorldsUI")->findChild("Worlds")->removeAll();

	auto saves = engine.getWorldsList();
	float yPos = 0.2f;
	int index = 0;
	for (auto& saveName : saves) {
		auto btn = engine.scene.interface->findChild("WorldsUI")->addChild<Button>(saveName.first, globalFont, engine.worldManagment.getWorldName(saveName.second), 20, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
		btn->position = sf::Vector2f(0.25f, yPos);
		btn->size = sf::Vector2f(0.3f, 0.05f);
		btn->alignmentX = 3;
		btn->alignmentY = 3;
		btn->zIndex = 5;
		btn->shape->setOutlineColor(sf::Color(190, 255, 220));
		btn->shape->setOutlineThickness(0);
		btn->onClick = [saveName](Engine& engine, std::shared_ptr<Button> button) {
			engine.selectedWorld = saveName.first;
			for (auto& obj : engine.scene.interface->findChild("WorldsUI")->findAll()) {
				if (auto ui = dynamic_pointer_cast<Button>(obj)) {
					ui->shape->setOutlineThickness(0);
				}
			}
			button->shape->setOutlineThickness(3);
		};
		yPos += 0.07f;
		index++;
	}
}

int main() {
	Engine engine;
	sf::Font globalFont = engine.loadFont("assets/fonts/tuffy.ttf");
	Renderer& renderer = engine.getRenderer();

	// -- DEV UI -- \\

	auto devUIFolder = engine.scene.interface->addChild<Container>("DevUI");
	devUIFolder->visible = false;

	auto DEV_username = devUIFolder->addChild<Text>("Username", globalFont, "None");
	DEV_username->position = sf::Vector2f(0.01f, 0.01f);
	DEV_username->size = sf::Vector2f(0.1f, 0.07f);
	DEV_username->alignmentX = 3;
	DEV_username->alignmentY = 3;
	auto DEV_stackText = devUIFolder->addChild<Text>("Stack", globalFont, "??? - KB");
	DEV_stackText->position = sf::Vector2f(0.01f, 0.06f);
	DEV_stackText->size = sf::Vector2f(0.1f, 0.07f);
	DEV_stackText->alignmentX = 3;
	DEV_stackText->alignmentY = 3;
	auto DEV_deltaTime = devUIFolder->addChild<Text>("DeltaTime", globalFont, "???");
	DEV_deltaTime->position = sf::Vector2f(0.01f, 0.11f);
	DEV_deltaTime->size = sf::Vector2f(0.1f, 0.07f);
	DEV_deltaTime->alignmentX = 3;
	DEV_deltaTime->alignmentY = 3;
	auto DEV_chunks = devUIFolder->addChild<Text>("ChunksRender", globalFont, "???");
	DEV_chunks->position = sf::Vector2f(0.01f, 0.16f);
	DEV_chunks->size = sf::Vector2f(0.1f, 0.07f);
	DEV_chunks->alignmentX = 3;
	DEV_chunks->alignmentY = 3;

	// -- MAIN UI -- \\

	auto menuUIFolder = engine.scene.interface->addChild<Container>("MenuUI");
	auto worldsUIFolder = engine.scene.interface->addChild<Container>("WorldsUI");
	auto pauseUIFolder = engine.scene.interface->addChild<Container>("PauseUI");
	auto worldsFolder = worldsUIFolder->addChild<Container>("Worlds");
	engine.scene.interface->findChild("WorldsUI")->visible = false;
	engine.scene.interface->findChild("PauseUI")->visible = false;
	engine.scene.interface->findChild("MenuUI")->visible = true;

	auto backgroundImage = engine.scene.interface->addChild<Image>("BackgroundImage", "assets/images/Menu.jpg");
	backgroundImage->position = sf::Vector2f(0.0f, 0.0f);
	backgroundImage->size = sf::Vector2f(1.0f, 1.0f);
	backgroundImage->zIndex = 0;

	auto MENU_frame1 = menuUIFolder->addChild<Frame>("Frame1", sf::Color(255, 255, 255, 150));
	MENU_frame1->position = sf::Vector2f(0.1f, 0.05f);
	MENU_frame1->size = sf::Vector2f(0.8f, 0.8f);
	auto MENU_title = menuUIFolder->addChild<Text>("MenuTitle", globalFont, "- The Weeping Dawn -", 45);
	MENU_title->position = sf::Vector2f(0.3f, 0.1f);
	MENU_title->size = sf::Vector2f(0.4f, 0.07f);
	MENU_title->shape->setFillColor(sf::Color(255, 255, 255, 0));
	MENU_title->alignmentX = 3;
	MENU_title->alignmentY = 3;
	auto MENU_playBtn = menuUIFolder->addChild<Button>("Play", globalFont, "PLAY", 24, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	MENU_playBtn->position = sf::Vector2f(0.4f, 0.2f);
	MENU_playBtn->size = sf::Vector2f(0.2f, 0.05f);
	MENU_playBtn->zIndex = 2;
	MENU_playBtn->alignmentX = 3;
	MENU_playBtn->alignmentY = 3;
	MENU_playBtn->onClick = [&globalFont](Engine& engine, std::shared_ptr<Button> button) {
		updateWorldsList(engine, globalFont);
	};
	auto MENU_exitBtn = menuUIFolder->addChild<Button>("Play", globalFont, "EXIT", 24, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	MENU_exitBtn->position = sf::Vector2f(0.4f, 0.3f);
	MENU_exitBtn->size = sf::Vector2f(0.2f, 0.05f);
	MENU_exitBtn->zIndex = 2;
	MENU_exitBtn->alignmentX = 3;
	MENU_exitBtn->alignmentY = 3;
	MENU_exitBtn->onClick = [](Engine& engine, std::shared_ptr<Button> button) {
		engine.exit();
	};

	auto WORLDS_frame1 = worldsUIFolder->addChild<Frame>("Frame1", sf::Color(255, 255, 255, 150));
	WORLDS_frame1->position = sf::Vector2f(0.1f, 0.05f);
	WORLDS_frame1->size = sf::Vector2f(0.8f, 0.8f);
	auto WORLDS_frame2 = worldsUIFolder->addChild<Frame>("Frame2", sf::Color(0, 0, 0, 150));
	WORLDS_frame2->position = sf::Vector2f(0.1725f, 0.17f);
	WORLDS_frame2->size = sf::Vector2f(0.65f, 0.65f);
	WORLDS_frame2->zIndex = 2;
	auto WORLDS_title = worldsUIFolder->addChild<Text>("WorldsTitle", globalFont, "- Worlds -", 45);
	WORLDS_title->position = sf::Vector2f(0.3f, 0.1f);
	WORLDS_title->size = sf::Vector2f(0.4f, 0.07f);
	WORLDS_title->shape->setFillColor(sf::Color(255, 255, 255, 0));
	WORLDS_title->alignmentX = 3;
	WORLDS_title->alignmentY = 3;
	WORLDS_title->zIndex = 5;
	auto WORLDS_selectBtn = worldsUIFolder->addChild<Button>("Select", globalFont, "SELECT", 24, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	WORLDS_selectBtn->position = sf::Vector2f(0.24f, 0.83f);
	WORLDS_selectBtn->size = sf::Vector2f(0.1f, 0.05f);
	WORLDS_selectBtn->zIndex = 5;
	WORLDS_selectBtn->alignmentX = 3;
	WORLDS_selectBtn->alignmentY = 3;
	WORLDS_selectBtn->onClick = [](Engine& engine, std::shared_ptr<Button> button) {
		if (engine.selectedWorld != "") {
			engine.scene.interface->findChild("MenuUI")->visible = false;
			engine.scene.interface->findChild("WorldsUI")->visible = false;
			engine.scene.interface->findChild("BackgroundImage")->visible = false;
			engine.clearWorld();

			std::string filepath = "assets/data/worlds/" + engine.selectedWorld + ".json";
			if (engine.loadWorld(filepath)) {
				engine.spawnCharacter();
			}
			else {
				std::cerr << "Failed to load " << filepath << std::endl;
			}

			engine.currentDimension->chunkManager->clear();
		}
	};
	auto WORLDS_backBtn = worldsUIFolder->addChild<Button>("Back", globalFont, "BACK", 24, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	WORLDS_backBtn->position = sf::Vector2f(0.12f, 0.83f);
	WORLDS_backBtn->size = sf::Vector2f(0.1f, 0.05f);
	WORLDS_backBtn->zIndex = 5;
	WORLDS_backBtn->alignmentX = 3;
	WORLDS_backBtn->alignmentY = 3;
	WORLDS_backBtn->onClick = [](Engine& engine, std::shared_ptr<Button> button) {
		engine.scene.interface->findChild("MenuUI")->visible = true;
		engine.scene.interface->findChild("WorldsUI")->visible = false;
	};
	auto WORLDS_createBtn = worldsUIFolder->addChild<Button>("Create", globalFont, "CREATE NEW WORLD", 20, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	WORLDS_createBtn->position = sf::Vector2f(0.36f, 0.83f);
	WORLDS_createBtn->size = sf::Vector2f(0.1f, 0.05f);
	WORLDS_createBtn->zIndex = 5;
	WORLDS_createBtn->alignmentX = 3;
	WORLDS_createBtn->alignmentY = 3;
	WORLDS_createBtn->onClick = [&globalFont](Engine& engine, std::shared_ptr<Button> button) {
		engine.clearWorld();

		std::random_device rd;
		int newSeed = rd();
		newSeed = std::abs(newSeed);

		std::string basePath = "assets/data/worlds/";
		std::string baseName = "NewWorld";
		std::string baseStringName = "New World";
		std::string stringName = baseStringName;
		std::string filename = basePath + baseName + ".json";
		int counter = 1;

		while (std::filesystem::exists(filename)) {
			filename = basePath + baseName + "_" + std::to_string(counter) + ".json";
			stringName = baseStringName + " " + std::to_string(counter);
			counter++;
		}
		if (!engine.worldManagment.createNewWorld(filename, stringName, newSeed)) {
			std::cerr << "Failed to create a new world" << std::endl;
			return;
		}

		updateWorldsList(engine, globalFont);
	};

	// -- ESCAPE MENU -- \\

	auto PAUSE_frame1 = pauseUIFolder->addChild<Frame>("Frame1", sf::Color(0, 0, 0, 200));
	PAUSE_frame1->position = sf::Vector2f(0.f, 0.f);
	PAUSE_frame1->size = sf::Vector2f(1.f, 1.f);
	auto PAUSE_title = pauseUIFolder->addChild<Text>("PausedText", globalFont, "- Paused -", 45, sf::Color(255, 255, 255));
	PAUSE_title->position = sf::Vector2f(0.3f, 0.1f);
	PAUSE_title->size = sf::Vector2f(0.4f, 0.07f);
	PAUSE_title->shape->setFillColor(sf::Color(255, 255, 255, 0));
	PAUSE_title->alignmentX = 3;
	PAUSE_title->alignmentY = 3;
	PAUSE_title->zIndex = 2;
	auto PAUSE_exitBtn = pauseUIFolder->addChild<Button>("ExitGame", globalFont, "Exit the Game", 25, sf::Color(255, 255, 255), sf::Color(150, 100, 180));
	PAUSE_exitBtn->position = sf::Vector2f(0.4f, 0.5f);
	PAUSE_exitBtn->size = sf::Vector2f(0.2f, 0.05f);
	PAUSE_exitBtn->alignmentX = 3;
	PAUSE_exitBtn->alignmentY = 3;
	PAUSE_exitBtn->zIndex = 2;
	PAUSE_exitBtn->onClick = [&globalFont](Engine& engine, std::shared_ptr<Button> button) {
		engine.worldManagment.save(engine.currentWorldPath, engine.dimensions);
		engine.exit();
	};

	// -- KEYBINDS -- \\

	engine.bindKey(sf::Keyboard::Key::F5, [](Engine& engine) {
		if (!engine.inMenu) {
			engine.setDebugMode(!engine.isDebuging());
			engine.scene.interface->findChild("DevUI")->visible = engine.isDebuging();
			if (engine.isDebuging()) engine.getRenderer().camera->cameraType = CameraType::FREECAM;
			else engine.getRenderer().camera->cameraType = CameraType::FIRST_PERSON;
		}
	});
	engine.bindKey(sf::Keyboard::Key::Z, [](Engine& engine) {
		if (!engine.inMenu) {
			engine.setMouseUnlocked(!mouseUnlocked);
			mouseUnlocked = !mouseUnlocked;
		}
	});
	engine.bindKey(sf::Keyboard::Key::Escape, [](Engine& engine) {
		if (!engine.inMenu) {
			mouseUnlocked = !mouseUnlocked;
			engine.paused = !engine.paused;
			engine.setMouseUnlocked(mouseUnlocked);
			engine.scene.interface->findChild("MenuUI")->visible = false;
			engine.scene.interface->findChild("WorldsUI")->visible = false;
			engine.scene.interface->findChild("PauseUI")->visible = engine.paused;
		}
	});

	// -- WORKSPACE -- \\

	auto centre = engine.scene.workspace->addChild<Object3D>("Centre", ObjectType::CUBE);
	centre->position = Vector3(0, 0, 0);
	centre->scale = Vector3(1, 1, 1);
	centre->isStatic = true;
	centre->color[0] = 1.f;
	centre->color[1] = 1.f;
	centre->color[2] = 1.f;
	engine.pushObj3D(centre);

	auto ground = engine.scene.workspace->addChild<Object3D>("Ground", ObjectType::CUBE);
	ground->position = Vector3(0, 0, 0);
	ground->scale = Vector3(10, 0.5f, 10);
	ground->isStatic = true;
	ground->color[0] = 1.f;
	ground->color[1] = 1.f;
	ground->color[2] = 1.f;
	engine.pushObj3D(ground);

	auto upGround = engine.scene.workspace->addChild<Object3D>("UpGround", ObjectType::CUBE);
	upGround->position = Vector3(3, 3, 3);
	upGround->scale = Vector3(5, 0.5f, 5);
	upGround->isStatic = true;
	upGround->color[0] = 1.f;
	upGround->color[1] = 1.f;
	upGround->color[2] = 1.f;
	engine.pushObj3D(upGround);

	auto test1 = engine.scene.workspace->addChild<Object3D>("Test1", ObjectType::CUBE);
	test1->position = Vector3(0, -5, 0);
	test1->scale = Vector3(1, 1, 1);
	test1->isStatic = true;
	test1->color[0] = 1.f;
	test1->color[1] = 1.f;
	test1->color[2] = 1.f;
	engine.pushObj3D(test1);

	auto sphere1 = engine.scene.workspace->addChild<Object3D>("Sphere1", ObjectType::SPHERE);
	sphere1->position = Vector3(-1, 5, -1);
	sphere1->scale = Vector3(1, 1, 1);
	sphere1->isStatic = false;
	sphere1->color[0] = 1.f;
	sphere1->color[1] = 1.f;
	sphere1->color[2] = 1.f;
	engine.pushObj3D(sphere1);

	for (int i = 0; i < 5; ++i) {
		auto cube = engine.scene.workspace->addChild<Object3D>("Cube", ObjectType::CUBE);
		cube->position = Vector3(static_cast<float>(i) * 2.0f - 4.0f, 12.0f + static_cast<float>(i) * 2.0f, static_cast<float>(i));
		cube->scale = Vector3(1, 1, 1);
		cube->color[0] = 0.2f + i * 0.1f;
		cube->color[1] = 0.5f;
		cube->color[2] = 0.8f;
		engine.pushObj3D(cube);
	}

	auto test5 = engine.scene.workspace->addChild<Object3D>("TEST5_BLENDER", ObjectType::CUSTOM);
	test5->loadOBJ("assets/models/test5.obj");
	test5->position = Vector3(1, 5, 1);
	test5->scale = Vector3(1, 1, 1);
	test5->color[0] = 0.8f;
	test5->color[1] = 0.4f;
	test5->color[2] = 0.2f;
	engine.pushObj3D(test5);


	/*
	auto rocketa = engine.scene.workspace->addChild<Object3D>("Cube", ObjectType::CUSTOM);
	rocketa->loadOBJ("assets/data/models/Rocketa.obj");
	rocketa->position = Vector3(-1, 5, -1);
	rocketa->scale = Vector3(1, 1, 1);
	rocketa->color[0] = 0.2f;
	rocketa->color[1] = 0.2f;
	rocketa->color[2] = 0.2f;
	engine.pushObj3D(rocketa);
	*/

	std::cout << "=== Engine Was Initialized ===" << std::endl;

	while (engine.isRunning()) {
		engine.update();
		engine.logic(); 

		DEV_deltaTime->text->setString(std::to_string(engine.getDeltaTime()) + " - DT");
		DEV_stackText->text->setString(std::to_string(engine.getAvailableStackSpace() / 1024) + " - KB Stack");
		if (engine.currentDimension) {
			DEV_chunks->text->setString(std::to_string(engine.currentDimension->chunkManager->getVisibleChunks().size()) + " Chunks");
		}

		engine.render();
	}

	return 0;
}