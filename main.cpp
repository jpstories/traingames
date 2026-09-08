#include <SFML/Graphics.hpp>
#include <iostream>


int main() {
	constexpr unsigned int WINDOW_WIDTH = 800;
	constexpr unsigned int WINDOW_HEIGHT = 600;

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "C++ Pong Mentor Edition");
	window.setFramerateLimit(60);

	constexpr float paddleWidth = 20.f;
	constexpr float paddleHeight = 100.f;
	constexpr float paddleSpeed = 8.f;

	// Левая ракетка (Игрок)
	sf::RectangleShape leftPaddle(sf::Vector2f(paddleWidth, paddleHeight));
	leftPaddle.setFillColor(sf::Color::White);
	leftPaddle.setPosition(50.f, (WINDOW_HEIGHT / 2.f) - (paddleHeight / 2.f));

	// Правая ракетка (ИИ)
	sf::RectangleShape rightPaddle(sf::Vector2f(paddleWidth, paddleHeight));
	rightPaddle.setFillColor(sf::Color::White);
	rightPaddle.setPosition(WINDOW_WIDTH - 50.f - paddleWidth, (WINDOW_HEIGHT / 2.f) - (paddleHeight / 2.f));

	// Настройка мяча
	constexpr float ballRadius = 10.f;
	sf::CircleShape ball(ballRadius);
	ball.setFillColor(sf::Color::White);
	ball.setOrigin(ballRadius, ballRadius); // переносит "точку привязки" с угла в центр мяча
	ball.setPosition(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);

	// Стартовая скорость движения мяча
	float ballVelocityX = 5.f;
	float ballVelocityY = 4.f;

	// Множитель ускорения мяча при каждом ударе
	constexpr float speedMultiplier = 1.05f;


	// Главный игровой цикл (1 итерация = 1 кадр)
	while (window.isOpen()) {
		sf::Event event; // Обработка системных событий
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Опрос клавиатуры игрока каждый кадр
		// y = 0 — это самый верхний край экрана
		// getPosition().y - верхняя координата ракетки
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && leftPaddle.getPosition().y > 0.f) {
			leftPaddle.move(0.f, -paddleSpeed); // если ниже, то можно двигать вверх
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && leftPaddle.getPosition().y + paddleHeight < WINDOW_HEIGHT) {
			leftPaddle.move(0.f, paddleSpeed); // если выше, то можно двигать вниз
		}

		ball.move(ballVelocityX, ballVelocityY);

		// Логика ракетки ИИ
		float targetY = ball.getPosition().y; // позиция мяча по y
		float paddleCenterY = rightPaddle.getPosition().y + (paddleHeight / 2.f); // центр ракетки

		// Если центр ракетки ниже цели — двигаемся вверх, если выше — вниз
		if (paddleCenterY > targetY + paddleSpeed) {
			if (rightPaddle.getPosition().y > 0.f) {
				rightPaddle.move(0.f, -paddleSpeed);
			}
		}
		else if (paddleCenterY < targetY - paddleSpeed) {
			if (rightPaddle.getPosition().y + paddleHeight < WINDOW_HEIGHT) {
				rightPaddle.move(0.f, paddleSpeed);
			}
		}

		// Физика, отскоки верх/низ
		if (ball.getPosition().y - ballRadius < 0.f) {
			ball.setPosition(ball.getPosition().x, ballRadius); // Корректируем позицию, чтобы не залипал
			ballVelocityY = -ballVelocityY; // Инвертируем скорость по Y
		}
		if (ball.getPosition().y + ballRadius > WINDOW_HEIGHT) {
			ball.setPosition(ball.getPosition().x, WINDOW_HEIGHT - ballRadius);
			ballVelocityY = -ballVelocityY;
		}

		// Физика, отскок от ракеток
		// Проверяем столкновение с левой ракеткой
		if (ball.getGlobalBounds().intersects(leftPaddle.getGlobalBounds())) {
			// Корректируем позицию мяча, чтобы он не застрял внутри ракетки
			ball.setPosition(leftPaddle.getPosition().x + paddleWidth + ballRadius, ball.getPosition().y);
			ballVelocityX = -ballVelocityX * speedMultiplier; // Отскок + ускорение
			ballVelocityY *= speedMultiplier;
		}

		// Проверяем столкновение с правой ракеткой
		if (ball.getGlobalBounds().intersects(rightPaddle.getGlobalBounds())) {
			// Корректируем позицию мяча
			ball.setPosition(rightPaddle.getPosition().x - ballRadius, ball.getPosition().y);
			ballVelocityX = -ballVelocityX * speedMultiplier; // Отскок + ускорение
			ballVelocityY *= speedMultiplier * 1.5;
		}

		// сброс при голе (лево / право)
		if (ball.getPosition().x < 0.f || ball.getPosition().x > WINDOW_WIDTH) {
			// Мяч улетел — возвращаем в центр поля
			ball.setPosition(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);
			// Сбрасываем скорость до базовой при голе
			ballVelocityX = (ballVelocityX > 0) ? -5.f : 5.f;
			ballVelocityY = (ballVelocityY > 0) ? 4.f : -4.f;
		}

		window.clear(sf::Color::Black); // Очищаем экран каждый кадр и красим снова
		window.draw(leftPaddle);		// Рисуем ракетку игрока
		window.draw(rightPaddle);		// Рисуем ракетку ИИ
		window.draw(ball);				// Рисуем Мяч
		window.display();				// Выводим кадр на экран
	}

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