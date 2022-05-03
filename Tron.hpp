#include <SFML/Graphics.hpp>
#include <random>
#include <bitset>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <json.hpp>
#include <Windows.h>

using nlohmann::json;

class Session
{
public:
    static constexpr auto width = 600ULL;
    static constexpr auto height = 480ULL;

    using socket_t = boost::asio::ip::tcp::socket;
    using field_t = std::bitset< width * height >;

    using socket_ptr_t = std::unique_ptr<socket_t>;

    static constexpr auto port = 15150;
    static constexpr auto max_connections = 10;
    static constexpr auto fps = 10U;
    static constexpr auto player_size = 3U;
    static constexpr auto player_speed = 4U; // ticks per frame

    static constexpr auto background_filename = "../images/background.jpg";
    static constexpr auto font_filename = "../fonts/FFF_Tusj.ttf";

    static constexpr auto title = "The Tron Game!";
    static constexpr auto start_text = "Use W, A, S and D to move your Tron Bike\nIf you're ready, tap Enter";
    static constexpr auto ready_text = "Waiting for your opponent...";
    static constexpr auto local_win_text = "Victory!!!";
    static constexpr auto remote_win_text = "Lose...";

    struct Player
    {
        enum class directions
        {
            left,
            right,
            up,
            down
        };
        Player() : Player(sf::Color::White, 0, 0, directions::down) {}

        Player(sf::Color color_, int x_, int y_, directions dir_) :
    		x(x_), y(y_), color(color_), dir(dir_) {}

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

        sf::Vector3f get_rgb() const
        {
            return sf::Vector3f(color.r, color.g, color.b);
        }
        

        int x;
        int y;
        sf::Color color;
        directions dir;
    };

private:
    enum class commands
    {
        skip,
        left,
        right,
        up,
        down,
        exit
    };

    Player local_player;
    Player remote_player;
    field_t field;
    bool local_is_winner;
    bool exit_flag;
    sf::Font main_font;
    sf::Sprite main_sprite;
    sf::RenderTexture render;
    sf::Sprite background;
    sf::RenderWindow window;
    sf::Text center_text;
    sf::Event event;

    socket_ptr_t socket_ptr;

public:
    Session() : field(false), exit_flag(false), local_is_winner(false),
				window(sf::VideoMode(width, height), title)
    {}

private:
    void run()
    {
        init_render();

        is_ready();

        wait_for_start();

        while (!exit_flag)
        {
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    exit_flag = true;
                    send_command(commands::exit);
                }
            }

            track_local_keyboard();

            track_remote_keyboard();

			frame();
        }
        finish();
    }

    void send_command(commands command) const
    {
        std::vector <uint8_t> bytes;

        bytes.push_back(static_cast<uint8_t>(command));

        write(*socket_ptr, boost::asio::buffer(bytes));
    }

    commands receive_command() const
    {
        boost::asio::streambuf buffer;
		std::istream input_stream(&buffer);

        read(*socket_ptr, buffer, boost::asio::transfer_exactly(1ULL));

        uint8_t command;

        input_stream >> command;

        return static_cast<commands>(command);
    }

    void track_local_keyboard()
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
	        if (local_player.dir != Player::directions::right)
	        {
	        	local_player.dir = Player::directions::left;
	        	send_command(commands::left);
	        }
        }
    	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    	{
    		if (local_player.dir != Player::directions::left)
    		{
    			local_player.dir = Player::directions::right;
    			send_command(commands::right);
    		}
    	}
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
	        if (local_player.dir != Player::directions::down)
	        {
	        	local_player.dir = Player::directions::up;
	        	send_command(commands::up);
	        }
        }
    	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    	{
    		if (local_player.dir != Player::directions::up)
    		{
    			local_player.dir = Player::directions::down;
    			send_command(commands::down);
    		}
    	}
    	else
    	{
            send_command(commands::skip);
    	}
    }

    void track_remote_keyboard()
	{
        switch (receive_command())
        {
        case commands::left:
            if (remote_player.dir != Player::directions::right)
                remote_player.dir = Player::directions::left;
            break;

        case commands::right:
            if (remote_player.dir != Player::directions::left)
                remote_player.dir = Player::directions::right;
            break;

        case commands::up:
            if (remote_player.dir != Player::directions::down)
                remote_player.dir = Player::directions::up;
            break;

        case commands::down:
            if (remote_player.dir != Player::directions::up)
                remote_player.dir = Player::directions::down;
            break;

        case commands::exit:
            exit_flag = true;

        case commands::skip:
            break;
        }
    }

    void update()
    {
        // collision
        if (field[local_player.x + height * (local_player.y % height)])
        {
            exit_flag = true;
        }
        else if (field[remote_player.x + height * (remote_player.y % height)])
        {
            local_is_winner = true;
            exit_flag = true;
        }

        // update field
        field[local_player.x + height * (local_player.y % height)] = true;
        field[remote_player.x + height * (remote_player.y % height)] = true;
    }

    void init_render()
    {
        center_text.setFont(main_font);
        center_text.setFillColor(sf::Color::White);
        center_text.setCharacterSize(16);
        center_text.setString(start_text);
        center_text.setPosition(width * 0.1, height / 2.0);

        main_font.loadFromFile(font_filename);
        
        window.setFramerateLimit(fps);

        sf::Texture texture;
        texture.loadFromFile(background_filename);
        background = sf::Sprite(background);

        render.create(width, height);
        render.setSmooth(true);
        main_sprite.setTexture(render.getTexture());
        render.clear();
        render.draw(background);
    }

    void frame()
    {
        for (auto i = 0U; i < player_speed; ++i)
        {
            local_player.tick();
            remote_player.tick();

            update();

            sf::CircleShape c(player_size);

            c.setPosition(local_player.x, local_player.y);
            c.setFillColor(local_player.color);
            render.draw(c);

            c.setPosition(remote_player.x, remote_player.y);
            c.setFillColor(remote_player.color);
            render.draw(c);

            render.display();
        }

        window.clear();
        window.draw(main_sprite);
        window.display();
    }

    void wait_for_start()
    {
        window.clear();
        window.draw(main_sprite);
        center_text.setString(ready_text);
        window.draw(center_text);
        window.display();

        while (receive_command() != commands::skip);
    }

    void init_settings()
    {
        local_player = Player(sf::Color::Red, 50, 50, Player::directions::down);
        remote_player = Player(sf::Color::Green, width - 50, height - 50, Player::directions::up);
    }

    void send_settings()
    {
        json settings;

        settings["local"]["x"] = local_player.x;
        settings["local"]["y"] = local_player.y;
        settings["local"]["c"] = local_player.color.toInteger();
        settings["local"]["d"] = local_player.dir;

        settings["remote"]["x"] = remote_player.x;
        settings["remote"]["y"] = remote_player.y;
        settings["remote"]["c"] = remote_player.color.toInteger();
        settings["remote"]["d"] = remote_player.dir;

        auto bson = json::to_bson(settings);
        bson.insert(std::begin(bson), std::size(bson));

        std::cout << boost::asio::write(*socket_ptr, boost::asio::buffer(bson));
    }

    void receive_settings()
    {
        boost::asio::streambuf buffer;
        std::istream input_stream(&buffer);
        std::vector<uint8_t> bson;

        try
        {
            uint8_t bytes_to_read = 0;

            if (read(*socket_ptr, buffer, boost::asio::transfer_exactly(1ULL)))
            {
                input_stream >> bytes_to_read;
            	if (bytes_to_read)
            	{
            		boost::asio::read(*socket_ptr, buffer, boost::asio::transfer_exactly(bytes_to_read));
            	}
            }
        }
        catch (const boost::system::system_error & e)
        {
	        if (e.code().value() == 2)
	        {
		        
	        }
            std::cerr << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            system("pause");

            exit(e.code().value());
        }

		json j(json::from_bson(std::vector <uint8_t>(buffers_begin(buffer.data()), buffers_end(buffer.data()))));

        std::cout << j << std::endl;

        remote_player.x = j["local"]["x"];
        remote_player.y = j["local"]["y"];
        remote_player.color = sf::Color(j["local"]["c"]);
        remote_player.dir = j["local"]["d"];

        local_player.x = j["remote"]["x"];
        local_player.y = j["remote"]["y"];
        local_player.color = sf::Color(j["remote"]["c"]);
        local_player.dir = j["remote"]["d"];
    }

    void is_ready()
    {
        window.clear();
        window.draw(main_sprite);
        window.draw(center_text);
        window.display();

        while (!sf::Keyboard::isKeyPressed(sf::Keyboard::Enter));

        send_command(commands::skip);
    }

    void finish()
    {
        if (local_is_winner)
            center_text.setString(local_win_text);
        else
            center_text.setString(remote_win_text);
        
        window.draw(center_text);

        window.display();

        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::KeyPressed)
            {
                window.close();
            }
        }
    }

public:
    void launch_server()
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);

        boost::asio::io_service io_service;

        try
        {
            boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());

            acceptor.bind(endpoint);

            acceptor.listen(max_connections);

            socket_ptr = std::make_unique<socket_t>(io_service);

            acceptor.accept(*socket_ptr);

            init_settings();

            send_settings();

            run();
        }
        catch (const boost::system::system_error& e)
        {
            std::cerr << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            exit(e.code().value());
        }
        catch (...)
        {
            std::cerr << "Error occured! Unknown error!" << std::endl;
            terminate();
        }
    }

    void start_client(const std::string & raw_ip_address)
    {
        try
        {
	        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(raw_ip_address), port);

        	boost::asio::io_service io_service;

        	socket_ptr = std::make_unique<socket_t>(io_service, endpoint.protocol());

        	socket_ptr->connect(endpoint);

            receive_settings();

            run();
        }
        catch (const boost::system::system_error& e)
        {
            std::cerr << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            exit(e.code().value());
        }
        catch (...)
        {
            std::cerr << "Error occured! Unknown error!" << std::endl;
            terminate();
        }
    }
};