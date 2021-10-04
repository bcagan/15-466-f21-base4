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
	unsigned int textureID;  
	glm::ivec2   size;       // (width, height) of glyph
	glm::ivec2   bearing;    // baseline to (left,top) offset
	unsigned int advance;    // Offset to advance to next glyph
}; //Glyphs will be mapped to the above after being created in the face
//From their, the buffer of glyphs will be turne dinto a buffer of above references
//Textuere will be generated one at a time

//https://learnopengl.com/In-Practice/Text-Rendering was used to build
//the above data type

struct TexInfo {
	unsigned int texture;
	unsigned int codepoint;
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

	//hexapod leg to wobble:
	Scene::Transform *cube = nullptr;
	glm::quat cube_rotation;

	glm::vec3 get_cube_position();

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > cube_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

	//Text info
	std::string currentText;
	int currentSpeaker = 0;
	FT_Face ft_face;
	hb_font_t* hb_font;
	std::vector<Glyph> curLine;
	std::vector<TexInfo> foundGlyph;
	unsigned int getTexture(unsigned int codepoint,bool *success);
	void createBuf(std::string text);
	void setFont(std::string fontfile);
	FT_Library ft_library;
	FT_Error ft_error;
	hb_buffer_t* hb_buffer;

	//To do
	void displayText();
	void getCurrentText();

	//Game State
	struct GameState {
		unsigned int currentTrack, char0, char1, chapter, mode;
		//mode 0 - auto, 1 - dialogue, 2 - contradiction
	};
	GameState gameState;
	bool continueDialogueRight = false;
	bool continueDialogueLeft = false;
	void updateDialogue();
	bool enterContradiction = false;
	bool endLine = false;

	//Dialogue
	struct Chapter {
		std::pair<unsigned int, unsigned int> contradiction;
		std::pair<unsigned int, unsigned int> end;
		std::vector<std::pair<std::string,uint8_t>> dialogue0;
		std::vector<std::pair<std::string,uint8_t>> dialogue1;
	};

	std::vector<Chapter> chapters;

	void loadDialogue(std::string fileName);
	void testPrint();

};
