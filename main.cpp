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

enum class GameState {
	Menu,
	Playing
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
		if (shape.getPosition().y + shape.getSize().y < GameConfig::WindowHeight) {
			shape.move(0.f, GameConfig::PaddleSpeed);
		}
	}

	void reset(float startX, float startY) {
		shape.setSize({ GameConfig::PaddleWidth, GameConfig::PaddleHeight });
		shape.setPosition(startX, startY);
	}

	// штраф за гол
	void shrink() {
		sf::Vector2f currentSize = shape.getSize();
		float newHeight = currentSize.y * 0.75f; // Уменьшаем высоту ракетки

		// Ограничитель, чтобы ракетка не исчезла совсем
		if (newHeight < 10.f) newHeight = 10.f;

		// Смещаем ракетку по Y, чтобы она визуально сжималась к центру, а не к верхнему краю
		float heightDiff = currentSize.y - newHeight;
		shape.move(0.f, heightDiff / 2.f);

		shape.setSize({ currentSize.x, newHeight });
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

	void updatePosition() { shape.move(velocity); }
	void invertY() { velocity.y = -velocity.y; }

	void bounceFromPaddle(float correctX) {
		shape.setPosition(correctX, shape.getPosition().y);
		velocity.x = -velocity.x * GameConfig::SpeedMultiplier;
		velocity.y *= static_cast<float>(GameConfig::SpeedMultiplier * 1.2);
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
		if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
			throw std::runtime_error("Ошибка загрузки шрифта arial.ttf");
		}

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

		// Интерфейс Главного Меню
		menuTitle.setFont(font);
		menuTitle.setString("C++ PONG");
		menuTitle.setCharacterSize(70);
		menuTitle.setFillColor(sf::Color::White);
		// Центрируем заголовок
		sf::FloatRect titleBounds = menuTitle.getLocalBounds();
		menuTitle.setOrigin(titleBounds.left + titleBounds.width / 2.f, titleBounds.top + titleBounds.height / 2.f);
		menuTitle.setPosition(GameConfig::WindowWidth / 2.f, 150.f);

		startButton.setFont(font);
		startButton.setString("Start Game");
		startButton.setCharacterSize(40);
		startButton.setFillColor(sf::Color::White);
		sf::FloatRect startBounds = startButton.getLocalBounds();
		startButton.setOrigin(startBounds.left + startBounds.width / 2.f, startBounds.top + startBounds.height / 2.f);
		startButton.setPosition(GameConfig::WindowWidth / 2.f, 320.f);

		exitButton.setFont(font);
		exitButton.setString("Exit");
		exitButton.setCharacterSize(40);
		exitButton.setFillColor(sf::Color::White);
		sf::FloatRect exitBounds = exitButton.getLocalBounds();
		exitButton.setOrigin(exitBounds.left + exitBounds.width / 2.f, exitBounds.top + exitBounds.height / 2.f);
		exitButton.setPosition(GameConfig::WindowWidth / 2.f, 420.f);

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
			if (event.type == sf::Event::Closed) {
				window.close();
			}

			// Обработка кликов мыши в Главном меню
			if (currentState == GameState::Menu && event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

					if (startButton.getGlobalBounds().contains(mousePos)) {
						resetGame();
						currentState = GameState::Playing; // Переключаемся в игру
					}
					else if (exitButton.getGlobalBounds().contains(mousePos)) {
						window.close(); // Выход из игры
					}
				}
			}
		}
	}

	void update() {
		if (currentState == GameState::Menu) {
			// Эффект наведения мыши (Hover)
			sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
			sf::Vector2f mousePos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));

			startButton.setFillColor(startButton.getGlobalBounds().contains(mousePos) ? sf::Color::Yellow : sf::Color::White);
			exitButton.setFillColor(exitButton.getGlobalBounds().contains(mousePos) ? sf::Color::Yellow : sf::Color::White);
			return;
		}

		// логика игры в состоянии Playing
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) leftPaddle.moveUp();
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) leftPaddle.moveDown();

		ball.updatePosition();

		// ИИ противника
		float targetY = ball.getPosition().y;
		// Используем getBounds().height вместо GameConfig::PaddleHeight
		float paddleCenterY = rightPaddle.getPosition().y + (rightPaddle.getBounds().height / 2.f);
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

		// Если гол игроку -> очко боту + штраф
		if (ball.getPosition().x < 0.f) {
			botScore++;
			rightPaddle.shrink();
			updateScoreText();
			scoreSound.play();
			checkWinCondition();
		}
		// Если гол боту -> очко игроку + штраф
		else if (ball.getPosition().x > GameConfig::WindowWidth) {
			playerScore++;
			leftPaddle.shrink();
			updateScoreText();
			scoreSound.play();
			checkWinCondition();
		}
	}

	void checkWinCondition() {
		if (playerScore >= 5 || botScore >= 5) {
			currentState = GameState::Menu; // Игра окончена, возвращаемся в меню
		}
		else {
			ball.reset(playerScore < botScore);
		}
	}

	void resetGame() {
		playerScore = 0;
		botScore = 0;
		updateScoreText();
		leftPaddle.reset(50.f, (GameConfig::WindowHeight / 2.f) - (GameConfig::PaddleHeight / 2.f));
		rightPaddle.reset(GameConfig::WindowWidth - 50.f - GameConfig::PaddleWidth, (GameConfig::WindowHeight / 2.f) - (GameConfig::PaddleHeight / 2.f));
		ball.reset(true);
	}

	void updateScoreText() {
		// C++20 форматирование для игровых UI элементов
		playerText.setString(std::format("{}", playerScore));
		botText.setString(std::format("{}", botScore));
	}

	void render() {
		window.clear(sf::Color::Black);

		if (currentState == GameState::Menu) {
			// Рисуем меню
			window.draw(menuTitle);
			window.draw(startButton);
			window.draw(exitButton);
		}
		else {
			// Рисуем разметку поля
			window.draw(centerLine);
			window.draw(playerText);
			window.draw(botText);

			window.draw(leftPaddle.getShape());
			window.draw(rightPaddle.getShape());
			window.draw(ball.getShape());
		}
		// Отрисовываем кадр на экран для любого состояния
		window.display();
	}

private:
	sf::RenderWindow window;
	Paddle leftPaddle;
	Paddle rightPaddle;
	Ball ball;

	// Переменная текущего состояния
	GameState currentState = GameState::Menu;

	// UI элементы
	sf::Font font;
	sf::Text playerText;
	sf::Text botText;
	sf::RectangleShape centerLine;

	// UI Меню
	sf::Text menuTitle;
	sf::Text startButton;
	sf::Text exitButton;

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