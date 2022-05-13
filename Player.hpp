#pragma once

class Player
{
public:

    enum class directions
    {
        left,
        right,
        up,
        down
    };

public:
    
    int width;
    int height;
    int x;
    int y;
    sf::Color color;
    directions dir;

public:

    Player() : Player(0, 0, 0, 0, sf::Color::White, directions::down) {}

    explicit Player(int width, int height, int x, int y, sf::Color color, directions dir) :
		width(width), height(height), x(x), y(y), color(color), dir(dir) {}

    void tick()
    {
        switch (dir)
        {
        case directions::up:
            --y;
            if (y < 0)
            {
                y = height - 1;
            }
            break;
        case directions::right:
            ++x;
            if (x > width)
            {
                x = 0;
            }
            break;
        case directions::down:
            ++y;
            if (y > height)
            {
                y = 0;
            }
            break;
        case directions::left:
            --x;
            if (x < 0)
            {
                x = width - 1;
            }
            break;
        }
    }
};