#include "UserInterface.h"

UserInterface::UserInterface(string vs, string fs, int width, int height, float brightness, string fontPath)
{
    _fontPath = fontPath;
    initShaders(vs, fs, width, height, brightness);
    initFreetype();
    generateCharacterTextures();
}

void UserInterface::updateUI(int fps, bool lost, bool won, double time, glm::vec3 color)
{
    renderFPS(fps, color);
    renderUserinterface(color);

    if (won) {
        renderWon(color);
    }
    else if (lost) {
        renderLost(color);
    }
    else {
        renderTime(time, color);
    }
}

void UserInterface::initShaders(string vs, string fs, int width, int height, float brightness)
{
    _width = width;
    _height = height;
    _shader = std::make_shared<Shader>(vs, fs);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
    _shader->use();
    _shader->setUniform("projection", projection);
    _shader->setUniform("brightness", brightness);

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UserInterface::renderUserinterface(glm::vec3 color)
{
    renderText("F1 - Wireframe, F2 - Culling, F11 - Toggle Fullscreen, ESC - Escape", 0.01 * _width, 0.95 * _height, 0.0005 * _height, color);
}

void UserInterface::renderFPS(int fps, glm::vec3 color)
{
    renderText("FPS: " + std::to_string(fps), 0.9 * _width, 0.95 * _height, 0.0005 * _height, color);
}

void UserInterface::renderLost(glm::vec3 color) {

    renderText("you failed :(",
        0.38 * _width, 0.5 * _height, 0.002 * _height, color);
}

void UserInterface::renderWon(glm::vec3 color) {

    renderText("yay, you won!",
        0.35 * _width, 0.5 * _height, 0.002 * _height, color);
}

void UserInterface::renderTime(double time, glm::vec3 color) {
    // get minutes and seconds of time left
    int mins = time / 60;
    int secs = int(time) % 60;
    int tenth = (time - 60 * mins - secs) * 10;
    renderText("Time left: " + std::to_string(mins) + ":" + std::to_string(secs) + "." + std::to_string(tenth),
        0.01 * _width, 0.02 * _height, 0.0005 * _height, color);
}

void UserInterface::renderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    _shader->use();
    _shader->setUniform("textColor", glm::vec3(color.x, color.y, color.z));
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindVertexArray(_vao);

    // iterate through all characters
    std::string::const_iterator c;
    float startx = x;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = _characters[*c];
        // for Debug texts also possible to set new lines
        if (*c == '\n') {
            y -= ch.Size.y * scale * 1.75f;
            x = startx;
        }
        else {
            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;
            // update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, _vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))

        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// initialize freetype
void UserInterface::initFreetype()
{
    if (FT_Init_FreeType(&_ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }
}

// generate the texture of the first 128 ascii characters
void UserInterface::generateCharacterTextures()
{
    FT_Face face;

    if (FT_New_Face(_ft, _fontPath.c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }
    else {

        FT_Set_Pixel_Sizes(face, 0, 48); // size of the characters
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

        // first 128 chars of ascii
        for (unsigned char c = 0; c < 128; c++)
        {
            // load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                face->glyph->advance.x
            };
            _characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // clear freetype's resources
        FT_Done_Face(face);
        FT_Done_FreeType(_ft);
    }
}

