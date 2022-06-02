#pragma once

#include "Utils.h"
#include "Shader.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class UserInterface
{
private:

	FT_Library _ft;
	string _fontPath;
	std::shared_ptr<Shader> _shader;
	unsigned int _vao, _vbo;
	int _width, _height;
	
	// store character in a struct so that we can query it using a map to render it
	struct Character {
		unsigned int TextureID;  // ID handle of the glyph texture
		glm::ivec2   Size;       // Size of glyph
		glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
		unsigned int Advance;    // Offset to advance to next glyph
	};

	std::map<char, Character> _characters;

	void generateCharacterTextures();

	void initFreetype();

	void initShaders(string vs, string fs, int width, int height, float brightness);

	void renderUserinterface(glm::vec3 color);

	void renderFPS(int fps, glm::vec3 color);

	void renderText(std::string text, float x, float y, float scale, glm::vec3 color);

	void renderLost(glm::vec3 color);

	void renderWon(glm::vec3 color);

public:

	// constructor
	UserInterface(string vs, string fs, int width, int height, float brightness, string fontPath);

	void updateUI(int fps, bool lost, bool won, glm::vec3 color);

	
};
