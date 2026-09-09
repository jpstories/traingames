#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <format>
#include <string>

// Чистая структура классов уменьшила количество лишних аллокаций в памяти, 
// а функции стали работать локально с полями классов, что всегда даёт буст к производительности.
// Потребление памяти снизилось со 107 до 88.

struct GameConfig {
	static constexpr unsigned int WindowWidth = 800;
	static constexpr unsigned int WindowHeight = 600;
	static constexpr float PaddleWidth = 20.f;
	static constexpr float PaddleHeight = 100.f;
	static constexpr float PaddleSpeed = 6.f;
	static constexpr float BallRadius = 10.f;
	static constexpr float SpeedMultiplier = 1.05f;
};

class Paddle {
public:
	Paddle(float startX, float startY) {
		shape.setSize({GameConfig::PaddleWidth, GameConfig::PaddleHeight});
		shape.setFillColor(sf::Color::White);
		shape.setPosition(startX, startY);
	}

	void moveUp() {
		if (shape.getPosition().y > 0.f) {
			shape.move(0.f, -GameConfig::PaddleSpeed);
		}
	}

	void moveDown() {
		if (shape.getPosition().y + GameConfig::PaddleHeight < GameConfig::WindowHeight) {
			shape.move(0.f, GameConfig::PaddleSpeed);
		}
	}

	// C++20: возвращаем константную ссылку на форму для безопасного чтения/отрисовки
	[[nodiscard]] const sf::RectangleShape& getShape() const { return shape; }
	[[nodiscard]] sf::FloatRect getBounds() const { return shape.getGlobalBounds(); }
	[[nodiscard]] sf::Vector2f getPosition() const { return shape.getPosition(); }

private:
	sf::RectangleShape shape;
};

class Ball {
public:
	Ball() {
		shape.setRadius(GameConfig::BallRadius);
		shape.setFillColor(sf::Color::White);
		shape.setOrigin(GameConfig::BallRadius, GameConfig::BallRadius);
		reset(true);
	}

	void updatePosition() {
		shape.move(velocity);
	}

	void invertY() { velocity.y = -velocity.y; }

	void bounceFromPaddle(float correctX) {
		shape.setPosition(correctX, shape.getPosition().y);
		velocity.x = -velocity.x * GameConfig::SpeedMultiplier;
		velocity.y *= GameConfig::SpeedMultiplier * 1.2;
	}

	void reset(bool toRight) {
		shape.setPosition(GameConfig::WindowWidth / 2.f, GameConfig::WindowHeight / 2.f);
		velocity.x = toRight ? 5.f : -5.f;
		velocity.y = 4.f;
	}

	[[nodiscard]] const sf::CircleShape& getShape() const { return shape; }
	[[nodiscard]] sf::FloatRect getBounds() const { return shape.getGlobalBounds(); }
	[[nodiscard]] sf::Vector2f getPosition() const { return shape.getPosition(); }

private:
	sf::CircleShape shape;
	sf::Vector2f velocity;
};

class Game {
public:
	Game()
		: window(sf::VideoMode(GameConfig::WindowWidth, GameConfig::WindowHeight), "C++ PONG AI")
		, leftPaddle(50.f, (GameConfig::WindowHeight / 2.f) - (GameConfig::PaddleHeight / 2.f))
		, rightPaddle(GameConfig::WindowWidth - 50.f - GameConfig::PaddleWidth, (GameConfig::WindowHeight / 2.f) - (GameConfig::PaddleHeight / 2.f))
	{
		window.setFramerateLimit(60);
		setupUI();
		setupAudio();
	}

	void run() {
		while (window.isOpen()) {
			processEvents();
			update();
			render();
		}
	}

private:
	void setupUI() {
		// Загружаем стандартный шрифт Arial из папки Windows
		if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {}

		playerText.setFont(font);
		playerText.setCharacterSize(50);
		playerText.setFillColor(sf::Color::White);
		// Смещаем немного левее центра
		playerText.setPosition(GameConfig::WindowWidth / 2.f - 100.f, 20.f);

		botText.setFont(font);
		botText.setCharacterSize(50);
		botText.setFillColor(sf::Color::White);
		// Смещаем немного правее центра
		botText.setPosition(GameConfig::WindowWidth / 2.f + 60.f, 20.f);

		updateScoreText();

		// Декоративная разделительная линия по центру поля
		centerLine.setSize({ 4.f, static_cast<float>(GameConfig::WindowHeight) });
		centerLine.setFillColor(sf::Color(100, 100, 100));
		centerLine.setPosition(GameConfig::WindowWidth / 2.f - 2.f, 0.f);
	}

	void setupAudio() {
		if (hitBuffer.loadFromFile("hit.wav")) {
			hitSound.setBuffer(hitBuffer);
		}
		if (scoreBuffer.loadFromFile("score.wav")) {
			scoreSound.setBuffer(scoreBuffer);
		}
	}

	void processEvents() {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}
	}

	void update() {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) leftPaddle.moveUp();
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) leftPaddle.moveDown();

		ball.updatePosition();

		// ИИ противника
		float targetY = ball.getPosition().y;
		float paddleCenterY = rightPaddle.getPosition().y + (GameConfig::PaddleHeight / 2.f);
		if (paddleCenterY > targetY + GameConfig::PaddleSpeed) rightPaddle.moveUp();
		else if (paddleCenterY < targetY - GameConfig::PaddleSpeed) rightPaddle.moveDown();

		// Отскок от стен верх/низ + звук
		if (ball.getPosition().y - GameConfig::BallRadius < 0.f || ball.getPosition().y + GameConfig::BallRadius > GameConfig::WindowHeight) {
			ball.invertY();
			hitSound.play();
		}

		// Отскок от ракеток + звук
		if (ball.getBounds().intersects(leftPaddle.getBounds())) {
			ball.bounceFromPaddle(leftPaddle.getPosition().x + GameConfig::PaddleWidth + GameConfig::BallRadius);
			hitSound.play();
		}
		if (ball.getBounds().intersects(rightPaddle.getBounds())) {
			ball.bounceFromPaddle(rightPaddle.getPosition().x - GameConfig::BallRadius);
			hitSound.play();
		}

		// Гол + звук гола
		if (ball.getPosition().x < 0.f) {
			// Гол игроку, очко боту
			botScore++;
			updateScoreText();
			scoreSound.play();
			ball.reset(true);
		}
		else if (ball.getPosition().x > GameConfig::WindowWidth) {
			// Гол боту, очко игроку
			playerScore++;
			updateScoreText();
			scoreSound.play();
			ball.reset(false);
		}
	}

	void updateScoreText() {
		// C++20 форматирование для игровых UI элементов
		playerText.setString(std::format("{}", playerScore));
		botText.setString(std::format("{}", botScore));
	}

	void render() {
		window.clear(sf::Color::Black);

		// Рисуем разметку поля
		window.draw(centerLine);
		window.draw(playerText);
		window.draw(botText);

		window.draw(leftPaddle.getShape());
		window.draw(rightPaddle.getShape());
		window.draw(ball.getShape());

		window.display();
	}

private:
	sf::RenderWindow window;
	Paddle leftPaddle;
	Paddle rightPaddle;
	Ball ball;

	// UI элементы
	sf::Font font;
	sf::Text playerText;
	sf::Text botText;
	sf::RectangleShape centerLine;

	sf::SoundBuffer hitBuffer;
	sf::SoundBuffer scoreBuffer;
	sf::Sound hitSound;
	sf::Sound scoreSound;

	int playerScore = 0;
	int botScore = 0;
};

int main() {
	Game game;
	game.run();
	return 0;
}

// У окна есть два «холста»: 
// 1 - видит игрок на мониторе(передний буфер)
// 2 - заднем буфере, программа втайне от игрока делает работу
// — очищает экран и рисует новые объекты

// Команда window.display() мгновенно меняет эти холсты местами.
// Игрок видит уже готовый, плавно отрисованный кадр.
// В этот момент SFML смотрит на настройку setFramerateLimit(60) 
// и искусственно притормаживает программу, если кадр выполнился 
// слишком быстро.