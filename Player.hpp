#pragma once

template < std::size_t Width, std::size_t Height >
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

private:
    
    int x;
    int y;
    sf::Color color;
    directions dir;

public:

    Player() : Player(0, 0, 0, 0, sf::Color::White, directions::down) {}

    explicit Player(int x, int y, sf::Color color, directions dir) :
		x(x), y(y), color(color), dir(dir) {}

    void tick()
    {
        switch (dir)
        {
        case directions::up:
            --y;
            if (y < 0)
            {
                y = Height - 1;
            }
            break;
        case directions::right:
            ++x;
            if (x > Width)
            {
                x = 0;
            }
            break;
        case directions::down:
            ++y;
            if (y > Height)
            {
                y = 0;
            }
            break;
        case directions::left:
            --x;
            if (x < 0)
            {
                x = Width - 1;
            }
            break;
        }
    }
};