#pragma once

#include <random>
#include <bitset>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <json.hpp>
#include <SFML/Graphics.hpp>

#include "Player.hpp"

using nlohmann::json;

template < std::size_t Width, std::size_t Height >
class Session
{
public:

    using player_t = Player<Width, Height>;
    using socket_t = boost::asio::ip::tcp::socket;
    using field_t = std::bitset< Width * Height >;

    using socket_ptr_t = std::unique_ptr<socket_t>;

    const int port_ = 15150;
    const int max_connections_ = 10;

private:

    const std::size_t fps_ = 60U;
    const std::size_t player_size_ = 3U;
    const std::size_t player_speed_ = 4U; // ticks per frame

    const std::string background_filename_ = "../images/background.jpg";
    const std::string font_filename_ = "../fonts/Amatic-Bold.ttf";

    const std::string title_ = "The Tron Game!";
    const std::string start_text_ = "Use W, A, S and D to move your Tron Bike\nIf you're ready, tap Enter";
    const std::string ready_text_ = "Waiting for your opponent...";
    const std::string local_win_text_ = "Victory!!!";
    const std::string remote_win_text_ = "Lose...";

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

    player_t local_player_;
    player_t remote_player_;
    field_t field_;
    bool local_is_winner_;
    bool exit_flag_;
    sf::Font main_font_;
    sf::Sprite main_sprite_;
    sf::RenderTexture render_;
    sf::Sprite background_;
    sf::RenderWindow window_;
    sf::Text center_text_;
    sf::Event event_;

    socket_ptr_t socket_;

public:
    explicit Session() : field_(false), local_is_winner_(false), exit_flag_(false),
				window_(sf::VideoMode(Width, Height), title_)
    {}

private:
    void run();

    void send_command(commands command) const
    {
        std::vector <uint8_t> bytes;

        bytes.push_back(static_cast<uint8_t>(command));

        write(*socket_, boost::asio::buffer(bytes));
    }

    commands receive_command() const
    {
        boost::asio::streambuf buffer;
		std::istream input_stream(&buffer);

        read(*socket_, buffer, boost::asio::transfer_exactly(1ULL));

        uint8_t command;

        input_stream >> command;

        return static_cast<commands>(command);
    }

    void track_local_keyboard()
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && 
				local_player_.dir != Player::directions::right)
        {
	        local_player_.dir = Player::directions::left;
	        send_command(commands::left);
        }
    	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && 
				local_player_.dir != Player::directions::left)
    	{
    		local_player_.dir = Player::directions::right;
    		send_command(commands::right);
    	}
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && 
				local_player_.dir != Player::directions::down)
        {
	        local_player_.dir = Player::directions::up;
	        send_command(commands::up);
        }
    	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && 
				local_player_.dir != Player::directions::up)
    	{
    		local_player_.dir = Player::directions::down;
    		send_command(commands::down);
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
            if (remote_player_.dir != Player::directions::right)
                remote_player_.dir = Player::directions::left;
            break;

        case commands::right:
            if (remote_player_.dir != Player::directions::left)
                remote_player_.dir = Player::directions::right;
            break;

        case commands::up:
            if (remote_player_.dir != Player::directions::down)
                remote_player_.dir = Player::directions::up;
            break;

        case commands::down:
            if (remote_player_.dir != Player::directions::up)
                remote_player_.dir = Player::directions::down;
            break;

        case commands::exit:
            exit_flag_ = true;

        case commands::skip:
            break;
        }
    }

    void update()
    {
        if (field_[local_player_.x + Width * (local_player_.y % Height)])
        {
            exit_flag_ = true;
            send_command(commands::exit);
        }
        else if (field_[remote_player_.x + Width * (remote_player_.y % Height)])
        {
            local_is_winner_ = true;
            exit_flag_ = true;
            send_command(commands::exit);
        }

        // update field
        field_[local_player_.x + Width * (local_player_.y % Height)] = true;
        field_[remote_player_.x + Width * (remote_player_.y % Height)] = true;
    }

    void init_render()
    {
        center_text_.setFont(main_font_);
        center_text_.setFillColor(sf::Color::White);
        center_text_.setCharacterSize(24U);
        center_text_.setString(start_text_);
        center_text_.setPosition(Width * 0.1, Height / 2.0);

        main_font_.loadFromFile(font_filename_);
        
        window_.setFramerateLimit(fps_);

        sf::Texture texture;
        texture.loadFromFile(background_filename_);
        background_ = sf::Sprite(texture);

        render_.create(Width, Height);
        render_.setSmooth(true);
        main_sprite_.setTexture(render_.getTexture());
        render_.clear();
        render_.draw(background_);
    }

    void frame()
    {
        for (auto i = 0U; i < player_speed_; ++i)
        {
            local_player_.tick();
            remote_player_.tick();

            update();

            sf::CircleShape c(player_size_);

            c.setPosition(local_player_.x, local_player_.y);
            c.setFillColor(local_player_.color);
            render_.draw(c);

            c.setPosition(remote_player_.x, remote_player_.y);
            c.setFillColor(remote_player_.color);
            render_.draw(c);

            render_.display();
        }

        window_.clear();
        window_.draw(main_sprite_);
        window_.display();
    }

    void init_settings()
    {
        local_player_ = player_t(50, 50, sf::Color::Red, player_t::directions::down);
        remote_player_ = player_t(Width - 50, Height - 50, sf::Color::Green, player_t::directions::up);

        std::random_device device;
        std::mt19937_64 generator(device());
        std::uniform_int_distribution<int> uid_x(50, Width - 50);
        std::uniform_int_distribution<int> uid_y(50, Height - 50);

        local_player_.x = uid_x(generator);
        local_player_.y = uid_y(generator);

        remote_player_.x = uid_x(generator);
        remote_player_.y = uid_y(generator);

        while ( std::abs(local_player_.x - remote_player_.x) < 100 )
        {
            remote_player_.x = uid_x(generator);
        }
        while (std::abs(local_player_.y - remote_player_.y) < 100)
        {
            remote_player_.y = uid_y(generator);
        }
    }

    void send_settings()
    {
        json settings;

        settings["local"]["x"] = local_player_.x;
        settings["local"]["y"] = local_player_.y;
        settings["local"]["c"] = local_player_.color.toInteger();
        settings["local"]["d"] = local_player_.dir;

        settings["remote"]["x"] = remote_player_.x;
        settings["remote"]["y"] = remote_player_.y;
        settings["remote"]["c"] = remote_player_.color.toInteger();
        settings["remote"]["d"] = remote_player_.dir;

        auto bson = json::to_bson(settings);
        bson.insert(std::begin(bson), std::size(bson));

        boost::asio::write(*socket_, boost::asio::buffer(bson));
    }

    void receive_settings()
    {
        boost::asio::streambuf buffer;
        std::istream input_stream(&buffer);
        std::vector<uint8_t> bson;

        try
        {

            if (read(*socket_, buffer, boost::asio::transfer_exactly(1ULL)))
            {
                uint8_t bytes_to_read = 0;
                input_stream >> bytes_to_read;
            	if (bytes_to_read)
            	{
            		boost::asio::read(*socket_, buffer, boost::asio::transfer_exactly(bytes_to_read));
            	}
            }
        }
        catch (const boost::system::system_error & e)
        {
	        if (e.code().value() == 2)
	        {
		        
	        }
            std::cerr << "Error occurred! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            system("pause");

            exit(e.code().value());
        }

		json j(json::from_bson(std::vector <uint8_t>(buffers_begin(buffer.data()), buffers_end(buffer.data()))));

        remote_player_.x = j["local"]["x"];
        remote_player_.y = j["local"]["y"];
        remote_player_.color = sf::Color(j["local"]["c"]);
        remote_player_.dir = j["local"]["d"];

        local_player_.x = j["remote"]["x"];
        local_player_.y = j["remote"]["y"];
        local_player_.color = sf::Color(j["remote"]["c"]);
        local_player_.dir = j["remote"]["d"];
    }

    void is_ready()
    {
        window_.clear();
        window_.draw(main_sprite_);
        window_.draw(center_text_);
        window_.display();

        std::cout << "Wait for Enter\n";

        while (window_.waitEvent(event_) && !sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {}

        send_command(commands::skip);
    }

    void wait_for_start()
    {
        window_.clear();
        window_.draw(main_sprite_);
        center_text_.setString(ready_text_);
        window_.draw(center_text_);
        window_.display();

        while (window_.waitEvent(event_) && receive_command() != commands::skip) {}
    }

    void finish()
    {
        if (local_is_winner_)
            center_text_.setString(local_win_text_);
        else
            center_text_.setString(remote_win_text_);
        
        window_.draw(center_text_);
        window_.display();

        while (window_.waitEvent(event_) && event_.type != sf::Event::KeyPressed) {}

        window_.close();

        socket_->close();
    }

public:
    void launch_server()
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port_);

        try
        {
            boost::asio::io_service io_service;

            boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());

            acceptor.bind(endpoint);

            acceptor.listen(max_connections_);

            socket_ = std::make_unique<socket_t>(io_service);

            acceptor.accept(*socket_);

            init_settings();

            send_settings();

            run();
        }
        catch (const boost::system::system_error& e)
        {
            std::cerr << "Error occurred! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            exit(e.code().value());
        }
        catch (...)
        {
            std::cerr << "Error occurred! Unknown error!" << std::endl;
            terminate();
        }
    }

    void start_client(const std::string & raw_ip_address)
    {
        try
        {
	        const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(raw_ip_address), port_);

        	boost::asio::io_service io_service;

        	socket_ = std::make_unique<socket_t>(io_service, endpoint.protocol());

        	socket_->connect(endpoint);

            receive_settings();

            run();
        }
        catch (const boost::system::system_error& e)
        {
            std::cerr << "Error occurred! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

            exit(e.code().value());
        }
        catch (...)
        {
            std::cerr << "Error occurred! Unknown error!" << std::endl;
            terminate();
        }
    }
};