#define  _CRT_SECURE_NO_WARNINGS

#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include<vector>
#include <iostream>
#include<fstream>
#include <stdlib.h>
#include <assert.h>
#include <array>
#include <algorithm>
#define PI 3.14159f

GLuint test_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > test_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("test.pnct"));
	test_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

void PlayMode::testPrint() {
	for (size_t c = 0; c < chapters.size(); c++) {
		Chapter chap = chapters[c];
		std::pair<unsigned int, unsigned int> thisCont = chap.contradiction;
		std::pair<unsigned int, unsigned int> thisEnd = chap.end;
		std::vector<std::pair<std::string, uint8_t>> dialogue0 = chap.dialogue0;
		std::vector<std::pair<std::string, uint8_t>> dialogue1 = chap.dialogue1;
		std::cout << "CHAPTER: \n";
		for (size_t dio = 0; dio < dialogue0.size(); dio++) {
			if (dio == thisCont.first) std::cout << " This is a contradiction: ";
			std::cout << dio << ": ";
			std::cout << dialogue0[dio].first;
			std::cout << std::endl;
		}
		std::cout << "At line " << thisEnd.first << std::endl;
		for (size_t dio = 0; dio < dialogue1.size(); dio++) {
			if (dio == thisCont.second) std::cout << " This is a contradiction: ";
			std::cout << dio << ": ";
			std::cout << dialogue1[dio].first;
			std::cout << std::endl;
		}
		std::cout << "At line " << thisEnd.second << std::endl;
	}
}

void PlayMode::loadDialogue(std::string fileName) {
	//To be added: Chapter WIN
	std::string properPath = data_path(fileName);
	chapters = std::vector<Chapter>();
	std::ifstream tempStream;
	tempStream.open(fileName, std::ios::in | std::ios::binary);
	if (tempStream.is_open()) {
		//State params (used to check formatting for errors)
		bool character = false;
		bool ended = true;
		bool chapter = false;
		bool readLines = false;
		char line[999];

		std::vector<std::pair<std::string, uint8_t>> dialogue;
		Chapter curChapter;

		while (tempStream.getline(line,999)) { 

			std::string inString = std::string(line);

			//Tokenizer based on https://www.geeksforgeeks.org/tokenizing-a-string-cpp/
			std::vector < std::string> tokens = std::vector<std::string>();
			char* temp = std::strtok(line, " ");
			while (temp != NULL) {
				tokens.push_back(std::string(temp));
				temp = std::strtok(NULL, " ");
			}
			bool linePick = false;
			if (readLines && !linePick) {
				linePick = true;
				if (tokens.size() < 3) std::runtime_error("Bad file format: Line dialogue missing information");
				unsigned int lineType = std::stoi(tokens[1]);
				size_t preamble = 2 + tokens[0].size() + tokens[1].size();
				std::string useLine = inString.substr(preamble, useLine.size() - preamble);
				std::pair<std::string, uint8_t> newLine;
				newLine.first = useLine;
				newLine.second = ((uint8_t)lineType);
				dialogue.push_back(newLine);
				if (tokens[0] == std::string("END")) {
					if (character) {
						curChapter.dialogue1 = dialogue;
						curChapter.end.second = (unsigned int)dialogue.size() - 1;
						chapters.push_back(curChapter);
						dialogue = std::vector<std::pair<std::string, uint8_t>>();
						Chapter nextChapter;
						curChapter = nextChapter;
						assert(curChapter.dialogue0 != chapters[0].dialogue0);
						ended = true;
						readLines = false;
					}
					else {
						curChapter.dialogue0 = dialogue;
						curChapter.end.first = (unsigned int)dialogue.size() - 1;
						dialogue = std::vector<std::pair<std::string, uint8_t>>();
						character = true;
						ended = false;
					}
				}
				else ended = false;
			}
			if (chapter && !linePick) {
				linePick = true;
				chapter = false;
				readLines = true;
				character = false;
				std::vector<std::pair<std::string, uint8_t>> dialogue = std::vector<std::pair<std::string, uint8_t>>();
				if (tokens.size() != 2) std::runtime_error("Bad file format: Misformatted contadictions line");
				curChapter.contradiction.first = std::stoi(tokens[0]);
				curChapter.contradiction.second = std::stoi(tokens[1]);
			}
			if (ended && !linePick) {
				linePick = true;
				if (strlen(line) < 6 || inString.substr(0, 6) != std::string("CHAPTER"))
					std::runtime_error("Bad file format: CHAPTER doesn't begin a chapter");
				ended = false;
				chapter = true;
			}
		}
		tempStream.close();
	}
	else std::runtime_error("Error opening data file");
}

unsigned int PlayMode::getTexture(unsigned int codepoint, bool* success) {
	for (auto whichTexture : foundGlyph) {
		if (whichTexture.codepoint == codepoint) {
			*success = true;
			return whichTexture.texture;
		}
	}
	*success = false;
	return 0;
}

Load< Scene > test_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("test.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = test_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = test_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

PlayMode::PlayMode() : scene(*test_scene) {


	hb_font = NULL;
	hb_buffer = NULL;
	FT_Error ft_error;

	//Set up font
	std::string fontString = std::string("OpenSans-Regular.ttf");

	if ((ft_error = FT_Init_FreeType(&ft_library)))
		abort();
	if ((ft_error = FT_New_Face(ft_library, fontString.c_str(), 0, &ft_face)))
		abort();
	if ((ft_error = FT_Set_Char_Size(ft_face, FONT_SIZE * 64, FONT_SIZE * 64, 0, 0)))
		abort();


	/* Create hb-ft font. */
	hb_font = hb_ft_font_create(ft_face, NULL);
	foundGlyph = std::vector<TexInfo>();

	loadDialogue("Contents.txt");

	/*for (auto& transform : scene.transforms) {
	}*/

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	cube_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_cube_position(), 10.0f);

	currentText = std::string("a"); //Will be quickly changed as a chapter is loaded in
	gameState.chapter = 0;
	gameState.char0 = 0;
	gameState.char1 = 0;
	gameState.currentTrack = 0;
	gameState.mode = GameState::AUTO;
}

PlayMode::~PlayMode() {
	FT_Done_FreeType(ft_library);
	FT_Done_Face(ft_face);
	hb_font_destroy(hb_font);
	hb_buffer_destroy(hb_buffer);
	hb_buffer = NULL;
	hb_font = NULL;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_x) {
			enterContradiction = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_1) {
			gameState.currentTrack = 0;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_2) {
			gameState.currentTrack = 1;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			continueDialogueRight = true;
			continueDialogueLeft = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_LEFT) {
			continueDialogueRight = false;
			continueDialogueLeft = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::updateDialogue() {
	if (continueDialogueRight) {
		continueDialogueRight = false;
		continueDialogueLeft = false;
		if (!gameState.currentTrack) {
			if (gameState.char0 == chapters[gameState.chapter].end.first) {
				gameState.char0 = 0;
			}
			else gameState.char0++;
		}
		else {
			if (gameState.char1 == chapters[gameState.chapter].end.second) {
				gameState.char1 = 0;
			}
			else gameState.char1++;
		}
	}
	else if(continueDialogueLeft) {
		continueDialogueRight = false;
		continueDialogueLeft = false;
		if (!gameState.currentTrack) {
			if (gameState.char0 == 0) {
				gameState.char0 = chapters[gameState.chapter].end.first;
			}
			else gameState.char0--;
		}
		else {
			if (gameState.char1 == 0) {
				gameState.char1 = chapters[gameState.chapter].end.second;
			}
			else gameState.char1--;
		}
	}
	if (gameState.currentTrack == 0) {
		currentText = chapters[gameState.chapter].dialogue0[gameState.char0].first;
		currentSpeaker = chapters[gameState.chapter].dialogue0[gameState.char0].second;
	}
	else {
		currentText = chapters[gameState.chapter].dialogue1[gameState.char1].first;
		currentSpeaker = chapters[gameState.chapter].dialogue1[gameState.char1].second;
	}
}

void PlayMode::update(float elapsed) {
	if (colorStatus < 1.0f) colorStatus += elapsed;
	else colorStatus = 1.0f;
	

	auto selectChar = [this]() {
		return !enterContradiction;
	};

	auto endGame = [this]() {
		currentText = "I should have known. That day would continue to haunt me as long as I lived. CHAPTER CLEAR.";
	};

	auto contradictionCheck = [this]() {
		if (enterContradiction) {
			colorStatus = 0.0f;
			enterContradiction = false;
			if (chapters[gameState.chapter].contradiction.first == gameState.char0 && chapters[gameState.chapter].contradiction.second == gameState.char1) {
				textColor = glm::vec3(0.125f, 1.0f, 0.125f);
				return true;
			}
			else {
				textColor = glm::vec3(1.0f, 0.125f, 0.125f);
				return false;
			}
		}
		textColor = glm::vec3(0.1f);
		return false;
	};
	if (gameState.mode == GameState::AUTO) { //mode 0 - ending, 1 - dialogue, 2 - contradiction
		if (continueDialogueLeft && (gameState.currentTrack == 0 && gameState.char0 == 0) || (gameState.currentTrack == 1 && gameState.char1 == 0)) continueDialogueLeft = false;
		if (continueDialogueRight && (gameState.currentTrack == 0 && chapters[gameState.chapter].end.first == gameState.char0 ||
			gameState.currentTrack == 1 && chapters[gameState.chapter].end.second == gameState.char1)) {
			continueDialogueRight = false;
			if (chapters.size() - 1 == gameState.chapter && gameState.currentTrack == 1) {
				endGame();
				gameState.mode = GameState::ENDING;
			}
			else if (chapters.size() - 1 != gameState.chapter){
				gameState.mode = GameState::DIALOGUE;
				gameState.chapter++;
				gameState.char0 = 0;
				gameState.char1 = 0;
			}
		}
		else 
			updateDialogue();
	}
	else if (gameState.mode == GameState::DIALOGUE) {
		if (selectChar()) {
			updateDialogue();
		}
		else {
			if (contradictionCheck()) {
				gameState.mode = GameState::CONTRADICTION;
				gameState.chapter++;
				gameState.char0 = 0;
				gameState.char1 = 0;
			}
		}
	}
	else if(gameState.mode == GameState::CONTRADICTION) {
		if (continueDialogueLeft &&( (gameState.currentTrack == 0 && gameState.char0 == 0) || (gameState.currentTrack == 1 && gameState.char1 == 0))) continueDialogueLeft = false;
		if (continueDialogueRight && (gameState.currentTrack == 0 && chapters[gameState.chapter].end.first == gameState.char0 ||
			gameState.currentTrack == 1 && chapters[gameState.chapter].end.second == gameState.char1)) {
			continueDialogueRight = false;
			gameState.chapter++;
			gameState.char0 = 0;
			gameState.char1 = 0;
			if (chapters.size() - 1 != gameState.chapter) gameState.mode = GameState::DIALOGUE;
			else {
				gameState.mode = GameState::AUTO;
			}
		}
		updateDialogue();
	}

	continueDialogueRight = false;

	//move sound to follow leg tip position:
	cube_loop->set_position(get_cube_position(), 1.0f / 60.0f);

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.



	scene.draw(*camera);

	std::string currentSubText = currentText;
	size_t startInd = 0;
	size_t level = 0;
	size_t usableSize = currentText.size() - 1; // Note that the text is being fed a carrige return, from notepad
		//Which is causing a filler character to appear. Decrementing the length skips this
	bool needHyp = false;
	//Code to find good spot for new line
	while (startInd < usableSize) {
		size_t strSize = 0;
		while (strSize < usableSize - startInd && strSize < 49) {
			strSize++;
		}
		bool foundSpace = false;
		size_t numCheck = 0;
		while (!foundSpace && strSize < usableSize - startInd - 1 && numCheck < 10) {
			if (currentText[startInd + strSize + 1] == ' ') { foundSpace = true; strSize++; }
			strSize++;
			numCheck++;
		}
		if (strSize == usableSize - startInd - 1) {
			strSize++; //If one character left, include
		}
		if (strSize >= usableSize - startInd) {
			foundSpace = true;
		}
		currentSubText = currentText.substr(startInd, strSize);
		if (needHyp) {
			currentSubText = std::string("-").append(currentSubText);
			needHyp = false;
		}
		if (!foundSpace) {
			needHyp = true;
			currentSubText = currentSubText.append("-");
		}
		displayText(currentSubText,level);
		startInd += strSize;
		level++;
	}



	GL_ERRORS();


	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}


void PlayMode::createBuf(std::string text) {
	//https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c was
	//references for this code
	hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8(hb_buffer, text.c_str(), (int)text.length(), 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);
	assert(hb_buffer != NULL);

	/* Shape it! */
	hb_shape(hb_font, hb_buffer, NULL, 0);

	/* Get glyph information and positions out of the buffer. */
	size_t len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t* info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);


	//Load buffer into glyph vector
	curLine.clear();
	curLine.reserve(len);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (size_t c = 0; c < len; c++) {
		Glyph newGlyph;
		curLine.push_back(newGlyph);
		bool success = false;
		if (FT_Load_Glyph(ft_face, info[c].codepoint, FT_LOAD_RENDER)) {
			std::runtime_error("Glyph was unable to load");
		}
		unsigned int texId = getTexture(info[c].codepoint, &success);
		if (!success) {
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				ft_face->glyph->bitmap.width,
				ft_face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				ft_face->glyph->bitmap.buffer
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			curLine[c].textureID = texture;
			curLine[c].size = glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows);
			TexInfo newInfo;
			newInfo.codepoint = info[c].codepoint;
			newInfo.texture = texture;
			foundGlyph.push_back(newInfo);
		}
		else {
			curLine[c].textureID = texId;
			curLine[c].size = glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows);
		}
		assert(curLine[c].textureID != 0);
		curLine[c].bearing = glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top);
		curLine[c].advance = (unsigned int)(pos[c].x_advance / 64.f);
	}
}



void PlayMode::displayText(std::string inText,size_t level) { //Also uses https://learnopengl.com/In-Practice/Text-Rendering
	
	std::cout << "level " << level << " text size " << inText.size() << std::endl;

	//Create glyphs
	createBuf(inText);


	//Set variables
	float x = 0.0f;
	float y = 0.0f;
	float scale = 0.015f;
	glm::vec3 posOffset = gameState.currentTrack == 0 ? glm::vec3(-15.0f, 10.0f, 10.0f) : glm::vec3(5.0f, 10.0f, 10.0f);
	glm::vec3 color = glm::vec3(0.3f,1.0f,1.0f)*colorStatus + (1.f-colorStatus)*textColor;
	if(currentSpeaker == 1) color = glm::vec3(1.0f, 7.f, 0.1f) * colorStatus + (1.f - colorStatus) * textColor;

	GLuint VAO, VBO;
	glGenBuffers(1, &VBO);


	struct Vertex {
		glm::vec4 Position;
		glm::vec3 Normal;
		glm::vec4 Color;
		glm::vec2 TexCoord;
	};
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
	GLint location = glGetAttribLocation(lit_color_texture_program->program, "Position");
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	location = glGetAttribLocation(lit_color_texture_program->program, "Normal");
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(4 * sizeof(float)));
	location = glGetAttribLocation(lit_color_texture_program->program, "Color");
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(7 * sizeof(float)));
	location = glGetAttribLocation(lit_color_texture_program->program, "TexCoord");
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(11 * sizeof(float)));
	GL_ERRORS();

	//Should be generealized to its own function later

	assert(!curLine.empty());
	posOffset += glm::vec3(0.f, 0.0f, -1.f*(float) level);
	for (size_t g = 0; g < curLine.size(); g++) {
		Glyph glyph = curLine[g];

		//Create plane mesh
		float xPos = x + glyph.bearing.x * scale;
		float yPos = y - (glyph.size.y - glyph.bearing.y) * scale;
		float width = glyph.size.x * scale;
		float height = glyph.size.y * scale;
		std::array<Vertex,6> vertices;

		vertices[0].Position = glm::vec4(xPos, yPos + height, 0.f, 1.0f);
		vertices[1].Position = glm::vec4(xPos, yPos, 0.f, 1.0f);
		vertices[2].Position = glm::vec4(xPos + width, yPos, 0.f, 1.0f);
		vertices[3].Position = glm::vec4(xPos, yPos + height, 0.f, 1.0f);
		vertices[4].Position = glm::vec4(xPos + width, yPos, 0.f, 1.0f);
		vertices[5].Position = glm::vec4(xPos + width, yPos + height, 0.f, 1.0f);
		//Im lazy
		for (size_t c = 0; c < vertices.size(); c++) {
			std::swap(vertices[c].Position.z, vertices[c].Position.y);
			vertices[c].Position += glm::vec4(posOffset,1.0f);
		} 

		for (size_t c = 0; c < 6; c++) {
			vertices[c].Normal = glm::vec3(0.0f, 0.0f, 1.0f);
			vertices[c].Color = glm::vec4(0, 1.0f, 1.0f, 1.0f);
		}
		vertices[0].TexCoord = glm::vec2(0.0f, 0.0f);
		vertices[1].TexCoord = glm::vec2(0.0f, 1.0f);
		vertices[2].TexCoord = glm::vec2(1.0f, 1.0f);
		vertices[3].TexCoord = glm::vec2(0.0f, 0.0f);
		vertices[4].TexCoord = glm::vec2(1.0f, 1.0f);
		vertices[5].TexCoord = glm::vec2(1.0f, 0.0f);

		glUseProgram(lit_color_texture_program->program);

		Scene::Camera camera = scene.cameras.front();
		assert(camera.transform);
		Scene::Transform newTransform;

		//newTransform.rotation = glm::normalize(newTransform.rotation * glm::angleAxis(2.f * PI / 4.f, glm::vec3(1.0f, 0.0f, 0.0f)) 
		//	* glm::angleAxis(2.f * PI / 4.f, glm::vec3(0.0f, 1.0f, 0.0f)));
		glm::mat4x3 object_to_world = newTransform.make_local_to_world();
		glm::mat4 world_to_clip = camera.make_projection() * glm::mat4(camera.transform->make_world_to_local());
		glm::mat4x3 world_to_light = glm::mat4x3(1.0f);
		glm::mat4 object_to_clip = world_to_clip * glm::mat4(object_to_world);;
		glUniformMatrix4fv(glGetUniformLocation(lit_color_texture_program->program, "OBJECT_TO_CLIP"), 1, GL_FALSE, glm::value_ptr(object_to_clip));
		GL_ERRORS();
		glm::mat4x3 object_to_light = world_to_light * glm::mat4(object_to_world);;
		GL_ERRORS();
		glUniformMatrix4x3fv(glGetUniformLocation(lit_color_texture_program->program, "OBJECT_TO_LIGHT"), 1, GL_FALSE, glm::value_ptr(object_to_light));
		GL_ERRORS();
		glm::mat3 normal_to_light = glm::inverse(glm::transpose(glm::mat3(object_to_light)));
		GL_ERRORS();
		glUniformMatrix3fv(glGetUniformLocation(lit_color_texture_program->program, "NORMAL_TO_LIGHT"), 1, GL_FALSE, glm::value_ptr(normal_to_light));
		GL_ERRORS();


		//upload vertices to vertex_buffer:
		glUniform1i(glGetUniformLocation(lit_color_texture_program->program, "TEXT_BOOL"), 1);
		GL_ERRORS();
		glUniform3fv(glGetUniformLocation(lit_color_texture_program->program, "TEXT_COLOR"), 1, glm::value_ptr(color));
		GL_ERRORS();

		glActiveTexture(GL_TEXTURE0);
		GL_ERRORS();
		glBindTexture(GL_TEXTURE_2D, glyph.textureID);
		GL_ERRORS();
		if(g == 0) glUniform1i(glGetUniformLocation(lit_color_texture_program->program, "TEX"), 0);
		GL_ERRORS();

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size()*sizeof(Vertex), vertices.data()); //upload vertices array

		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (glyph.advance) * scale;

	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

}

glm::vec3 PlayMode::get_cube_position() {
	//the vertex position here was read from the model in blender:
	return glm::vec3(1.0f);
}
