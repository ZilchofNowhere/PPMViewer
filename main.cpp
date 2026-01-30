#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>

#include <ios>
#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <unistd.h>
#include <pwd.h>

typedef int tmpColor_t;

int closestMultTo(int target, int num) {
    if (target % num == 0) {
        return target / num;
    }

    bool lastChanged = 1; // 0 is left, 1 is right
    int under = -1, over = -1, answer, left = target - num, right = target + num;
    while (left < right) {
        if (left % num == 0) {
            under = left;
            lastChanged = 0;
        }

        if (right % num == 0) {
            over = right;
            lastChanged = 1;
        }

        left++;
        right--;
    }

    answer = lastChanged ? over : under;

    return answer / num;
}

int idealPixelSize(int width, int height) {
    // checking if the image is larger than our target window size
    if (std::max(width, height) >= 800) {
        return 1;
    }
    // ideal window size is 800
    constexpr int target = 800;
    return closestMultTo(target, std::max(width, height));
}

// Color and pixel management
class Pixel {
public:
    Uint8 red;
    Uint8 green;
    Uint8 blue;

    static Uint32 getHexColor(const Uint8 red, const Uint8 green, const Uint8 blue) {
        return red * 0x10000 + green * 0x100 + blue;
    }

    Uint32 getHexColor() const {
        return this->red * 0x10000 + green * 0x100 + blue;
    }

    void setColors(const Uint8 red, const Uint8 green, const Uint8 blue) {
        this->red = red;
        this->green = green;
        this->blue = blue;
    }

    void setMonochrome(const Uint8 value) {
        this->red = value;
        this->green = value;
        this->blue = value;
    }

    void clear() {
        this->red = this->green = this->blue = 0;
    }

    std::string toString() const {
        return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
    }
};

Uint8 convertColor(const int color, const int maxColor) {
    if (color == 0) return 0;
    auto _color = static_cast<unsigned short>(color);
    auto _maxColor = static_cast<unsigned short>(maxColor);
    return (Uint8)((float)(_color + 1) * 256 / (_maxColor + 1) - 1);
}

// Window size (subject to change)
constexpr int width = 300, height = 50;

// Handling quitting
void quit(SDL_Window* window) {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }
    }
    
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Parsing helper
std::string getNextToken(std::ifstream& file) {
    std::string content;
    while (file >> content) {
        if (content.empty()) continue;
        if (content[0] == '#') {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        return content;
    }
    return "";
}

// File dialog functions
static std::string filePath;
static volatile bool isFileSelectionDone = false;
static constexpr SDL_DialogFileFilter fileDialogFilter[] = {
    { "PPM files", "ppm" },
    { "All files", "*" },
};
static constexpr int nFilter = SDL_arraysize(fileDialogFilter);

static void SDLCALL openFileCallback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }
    
    SDL_Log("Full path to selected file: '%s'", *filelist);
    filePath = *filelist;

    if (filter < 0) {
        SDL_Log("The current platform does not support fetching "
                "the selected filter, or the user did not select"
                " any filter.");
    } else if (filter < SDL_arraysize(fileDialogFilter)) {
        SDL_Log("The filter selected by the user is '%s' (%s).",
                fileDialogFilter[filter].pattern, fileDialogFilter[filter].name);
    }
    isFileSelectionDone = true;
}

static void readPixels(std::vector<std::vector<Pixel>>& pixels, std::ifstream& image, const int width, const int height, const int maxColor, const bool monochrome, const bool binary) {
    Pixel pixel;
    tmpColor_t r, g, b;
    std::string sRed, sGreen, sBlue;

    if (!monochrome) {
        if (binary) {
            for (int row = 0; row < height; row++) {
                pixels[row].reserve(width);
                for (int col = 0; col < width; col++) { // if maxColor < 256 then each color val will be 1 byte, else 2 bytes
                    unsigned char color[3];
                    image.read((char*)color, 3);

                    pixel.blue = convertColor(color[0], maxColor);
                    pixel.red = convertColor(color[1], maxColor);
                    pixel.green = convertColor(color[2], maxColor);

                    pixels[row].push_back(pixel);
                    pixel.clear();
                }
            }
        } else {
            for (int row = 0; row < height; row++) {
                pixels[row].reserve(width);
                for (int col = 0; col < width; col++) {
                    sRed = getNextToken(image);
                    sGreen = getNextToken(image);
                    sBlue = getNextToken(image);

                    r = std::stoi(sRed);
                    g = std::stoi(sGreen);
                    b = std::stoi(sBlue);

                    pixel.red = convertColor(r, maxColor);
                    pixel.green = convertColor(g, maxColor);
                    pixel.blue = convertColor(b, maxColor);

                    pixels[row].push_back(pixel);
                    pixel.clear();
                }
            }
        }
    } else {
        for (int row = 0; row < height; row++) {
            pixels[row].reserve(width); // allocates space but doesn't initialize
            for (int col = 0; col < width; col++) {
                sRed = getNextToken(image); // not actually the color red but why create another variable

                r = std::stoi(sRed);

                pixel.setMonochrome(convertColor(r, maxColor));

                pixels[row].push_back(pixel);
                pixel.clear();
            }
        }
    }
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Unable to init video: %s", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("PPMViewer", width, height, SDL_WINDOW_METAL);
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    
    // Opening the file
    char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    SDL_ShowOpenFileDialog(openFileCallback, nullptr, window, fileDialogFilter, nFilter, std::strcat(homedir, "/Pictures/"), false); // The final slash at the end of the path is VERY important
    while (!isFileSelectionDone) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
               isFileSelectionDone = true;
            }
        }
        
        SDL_Delay(10); // too unresponsive if given 100
    }

    if (filePath.empty()) {
        SDL_Log("No file path was provided.\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    std::ifstream image(filePath);
    if (!image.is_open()) {
        SDL_Log("File couldn't be opened.\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // File parsing
    int imageWidth = 0, imageHeight = 0;
    std::vector<std::vector<Pixel>> pixels;

    std::string header = getNextToken(image);

    bool isMonochrome = (header == "P1");
    bool isBinary = false;
    if (!isMonochrome && !(header == "P3" || header == "P6")) {
        SDL_Log("Couldn't recognize file type: %s\n", header.c_str());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    } else if (header == "P6") {
        isBinary = true;
    }
    std::cout << "Header is " << header << std::endl;
    std::cout << "File is " << (isMonochrome ? "monochrome" : "colorful") << std::endl;

    std::string sWidth = getNextToken(image);
    if (sWidth.empty()) {
        throw std::runtime_error("Missing window width");
    }

    std::string sHeight = getNextToken(image);
    if (sHeight.empty()) {
        throw std::runtime_error("Missing window height");
    }

    imageWidth = std::stoi(sWidth);
    std::cout << "Image width is " << imageWidth << std::endl;
    imageHeight = std::stoi(sHeight);
    std::cout << "Image height is " << imageHeight << std::endl;

    tmpColor_t maxColor = 1;
    if (!isMonochrome) {
        std::string sMax = getNextToken(image);
        if (sMax.empty()) {
            throw std::runtime_error("Missing max color");
        }
        maxColor = std::stoi(sMax);
        std::cout << "Max color value is " << maxColor << std::endl;
    }
    pixels.resize(imageHeight); // updates the vector to have the given size with its members initialized

    readPixels(pixels, image, imageWidth, imageHeight, maxColor, isMonochrome, isBinary);

    // Managing the window to draw
    int pixelWidth = idealPixelSize(imageWidth, imageHeight); // they are supposed to be squares anyway
    SDL_SetWindowSize(window, imageWidth * pixelWidth, imageHeight * pixelWidth);
    SDL_SetWindowTitle(window, filePath.c_str());
    surface = SDL_GetWindowSurface(window);
    SDL_LockSurface(surface); // for writing the pixels at once

    // Draw
    SDL_Rect pixel;
    for (int x = 0; x < imageHeight; x++) {
        for (int y = 0; y < imageWidth; y++) {
            pixel = SDL_Rect(y * pixelWidth, x * pixelWidth, pixelWidth, pixelWidth); // X&Y in reverse prob bcoz of the way SDL handles coordinates
            SDL_FillSurfaceRect(surface, &pixel, pixels[x][y].getHexColor());
        }
    }
    SDL_UnlockSurface(surface);
    SDL_UpdateWindowSurface(window);

    // Done
    std::cout << "Done reading!" << std::endl;

    quit(window);
}
