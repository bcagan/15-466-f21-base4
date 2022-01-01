#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <hb.h>
#include <hb-ft.h>
#include <string>

#define FONT_SIZE 36 //HarfBuzz implementation based on 
//https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c

struct Glyph {
	unsigned int textureID = 0;  
	glm::uvec2   size = glm::uvec2(0,0);       // (width, height) of glyph
	glm::ivec2   bearing = glm::ivec2(0,0);    // baseline to (left,top) offset
	unsigned int advance = 0;    // Offset to advance to next glyph
}; //Glyphs will be mapped to the above after being created in the face
//From their, the buffer of glyphs will be turne dinto a buffer of above references
//Textuere will be generated one at a time

//https://learnopengl.com/In-Practice/Text-Rendering was used to build
//the above data type

struct TexInfo {
	unsigned int texture = 0;
	unsigned int codepoint = 0; //Should turn into map
};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	glm::vec3 get_cube_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > cube_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

	//Text info
	std::string currentText;
	int currentSpeaker = 0;
	FT_Face ft_face = nullptr;
	hb_font_t* hb_font = nullptr;
	std::vector<Glyph> curLine;
	std::vector<TexInfo> foundGlyph;
	unsigned int getTexture(unsigned int codepoint,bool *success);
	void createBuf(std::string text);
	void setFont(std::string fontfile);
	FT_Library ft_library = nullptr;
	hb_buffer_t* hb_buffer = nullptr;
	void displayText(std::string inText, size_t level);
	void getCurrentText();

	//Game State
	struct GameState {
		enum Mode : unsigned int { AUTO, DIALOGUE, CONTRADICTION, ENDING };
		unsigned int currentTrack = 0, char0 = 0, char1 = 0, chapter = 0;
		Mode mode = AUTO;
		//spell check
	};
	GameState gameState;
	bool continueDialogueRight = false;
	bool continueDialogueLeft = false;
	void updateDialogue();
	bool enterContradiction = false;
	glm::vec3 textColor = glm::vec3(1.0f);
	float colorStatus = 1.0f; //0 color, 1 white

	//Dialogue
	struct Chapter {
		std::pair<unsigned int, unsigned int> contradiction = std::make_pair(0,0);
		std::pair<unsigned int, unsigned int> end = std::make_pair(0,0);
		std::vector<std::pair<std::string,uint8_t>> dialogue0;
		std::vector<std::pair<std::string,uint8_t>> dialogue1;
	};

	std::vector<Chapter> chapters;

	void loadDialogue(std::string fileName);
	void testPrint();

};
