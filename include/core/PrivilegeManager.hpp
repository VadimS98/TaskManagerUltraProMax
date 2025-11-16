#pragma once

#include <string>

enum class ElevationState
{
    NotElevated,   // обычный пользовательский токен
    Elevated,      // запущено с правами администратора
    Unknown        // не удалось определить (ошибка API)
};

class PrivilegeManager
{
public:
    // Проверить, запущен ли текущий процесс с правами администратора
    ElevationState GetCurrentProcessElevation() const;

    // Перезапустить текущий exe с UAC-поднятием (через ShellExecuteEx / runas).
    // arguments - что передать в командной строке новому процессу (можно оставить пустым).
    // Возвращает true, если запуск нового процесса был хотя бы инициирован.
    bool RestartWithAdmin(const std::wstring& arguments) const;
};