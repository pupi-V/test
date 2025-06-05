// Основной файл сервера - точка входа в приложение
import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "./routes";
import { setupVite, serveStatic, log } from "./vite";

// Создаем экземпляр Express приложения
const app = express();

// Настройка middleware для обработки JSON и URL-encoded данных
app.use(express.json()); // Парсинг JSON тел запросов
app.use(express.urlencoded({ extended: false })); // Парсинг форм

/**
 * Middleware для логирования API запросов
 * Отслеживает время выполнения и ответы API вызовов
 */
app.use((req, res, next) => {
  const start = Date.now();
  const path = req.path;
  let capturedJsonResponse: Record<string, any> | undefined = undefined;

  // Перехватываем JSON ответы для логирования
  const originalResJson = res.json;
  res.json = function (bodyJson, ...args) {
    capturedJsonResponse = bodyJson;
    return originalResJson.apply(res, [bodyJson, ...args]);
  };

  // Логируем после завершения запроса
  res.on("finish", () => {
    const duration = Date.now() - start;
    if (path.startsWith("/api")) {
      let logLine = `${req.method} ${path} ${res.statusCode} in ${duration}ms`;
      if (capturedJsonResponse) {
        logLine += ` :: ${JSON.stringify(capturedJsonResponse)}`;
      }

      // Обрезаем слишком длинные логи
      if (logLine.length > 80) {
        logLine = logLine.slice(0, 79) + "…";
      }

      log(logLine);
    }
  });

  next();
});

/**
 * Асинхронная функция запуска сервера
 * Настраивает все компоненты и запускает HTTP сервер
 */
(async () => {
  // Регистрируем API маршруты для работы с зарядными станциями
  const server = await registerRoutes(app);

  /**
   * Глобальный обработчик ошибок
   * Ловит все необработанные ошибки и возвращает унифицированный ответ
   */
  app.use((err: any, _req: Request, res: Response, _next: NextFunction) => {
    const status = err.status || err.statusCode || 500;
    const message = err.message || "Internal Server Error";

    // Отправляем клиенту информацию об ошибке
    res.status(status).json({ message });
    throw err;
  });

  /**
   * Настройка обслуживания статических файлов
   * В разработке используется Vite, в продакшене - Express
   * Важно настраивать Vite после всех остальных маршрутов
   * чтобы catch-all маршрут не перехватывал API вызовы
   */
  if (app.get("env") === "development") {
    await setupVite(app, server);
  } else {
    serveStatic(app);
  }

  /**
   * Запуск HTTP сервера
   * ВСЕГДА используем порт 5000 - это единственный незаблокированный порт
   * Сервер обслуживает как API, так и клиентское приложение
   */
  const port = 5000;
  server.listen({
    port,
    host: "0.0.0.0",    // Слушаем на всех интерфейсах
    reusePort: true,    // Позволяет перезапуск без ошибок "port in use"
  }, () => {
    log(`serving on port ${port}`);
  });
})();
