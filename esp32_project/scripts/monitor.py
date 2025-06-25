#!/usr/bin/env python3
"""
Расширенный монитор для ESP32 с фильтрацией и анализом логов
"""

import subprocess
import sys
import argparse
import re
import datetime

class ESP32Monitor:
    def __init__(self, port=None, baud=115200):
        self.port = port
        self.baud = baud
        self.filters = {
            'error': r'(ERROR|ОШИБКА|Failed|Ошибка)',
            'warning': r'(WARNING|WARN|Предупреждение)',
            'success': r'(SUCCESS|✅|успешно|готов)',
            'network': r'(WiFi|IP|WebSocket|HTTP)',
            'stations': r'(station|станци|charging|зарядн)'
        }
    
    def colorize_output(self, line):
        """Раскраска вывода для лучшей читаемости"""
        # ANSI коды цветов
        RED = '\033[91m'
        GREEN = '\033[92m'
        YELLOW = '\033[93m'
        BLUE = '\033[94m'
        MAGENTA = '\033[95m'
        CYAN = '\033[96m'
        WHITE = '\033[97m'
        RESET = '\033[0m'
        
        # Применение цветов
        if re.search(self.filters['error'], line, re.IGNORECASE):
            return f"{RED}{line}{RESET}"
        elif re.search(self.filters['warning'], line, re.IGNORECASE):
            return f"{YELLOW}{line}{RESET}"
        elif re.search(self.filters['success'], line, re.IGNORECASE):
            return f"{GREEN}{line}{RESET}"
        elif re.search(self.filters['network'], line, re.IGNORECASE):
            return f"{BLUE}{line}{RESET}"
        elif re.search(self.filters['stations'], line, re.IGNORECASE):
            return f"{CYAN}{line}{RESET}"
        else:
            return line
    
    def start_monitoring(self, filter_type=None, save_log=False):
        """Запуск мониторинга с опциональной фильтрацией"""
        cmd = ["pio", "device", "monitor"]
        if self.port:
            cmd.extend(["--port", self.port])
        cmd.extend(["--baud", str(self.baud)])
        
        print(f"🖥️  Запуск ESP32 Monitor")
        print(f"📡 Порт: {self.port or 'автоматический'}")
        print(f"⚡ Скорость: {self.baud}")
        if filter_type:
            print(f"🔍 Фильтр: {filter_type}")
        print("=" * 50)
        print("Нажмите Ctrl+C для остановки")
        print("=" * 50)
        
        log_file = None
        if save_log:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            log_filename = f"esp32_log_{timestamp}.txt"
            log_file = open(log_filename, 'w', encoding='utf-8')
            print(f"📁 Сохранение логов в: {log_filename}")
        
        try:
            process = subprocess.Popen(cmd, stdout=subprocess.PIPE, 
                                     stderr=subprocess.STDOUT, 
                                     universal_newlines=True, 
                                     bufsize=1)
            
            for line in iter(process.stdout.readline, ''):
                line = line.rstrip()
                if not line:
                    continue
                
                # Применение фильтра
                if filter_type and filter_type in self.filters:
                    if not re.search(self.filters[filter_type], line, re.IGNORECASE):
                        continue
                
                # Добавление временной метки
                timestamp = datetime.datetime.now().strftime("%H:%M:%S")
                formatted_line = f"[{timestamp}] {line}"
                
                # Раскраска и вывод
                colored_line = self.colorize_output(formatted_line)
                print(colored_line)
                
                # Сохранение в лог
                if log_file:
                    log_file.write(formatted_line + '\n')
                    log_file.flush()
        
        except KeyboardInterrupt:
            print("\n👋 Мониторинг остановлен")
        except Exception as e:
            print(f"❌ Ошибка мониторинга: {e}")
        finally:
            if log_file:
                log_file.close()
                print(f"📁 Лог сохранен в: {log_filename}")

def main():
    parser = argparse.ArgumentParser(description='Расширенный монитор ESP32')
    parser.add_argument('--port', '-p', help='COM порт ESP32')
    parser.add_argument('--baud', '-b', type=int, default=115200, 
                       help='Скорость передачи (по умолчанию: 115200)')
    parser.add_argument('--filter', '-f', 
                       choices=['error', 'warning', 'success', 'network', 'stations'],
                       help='Фильтр сообщений по типу')
    parser.add_argument('--save-log', '-s', action='store_true',
                       help='Сохранить лог в файл')
    
    args = parser.parse_args()
    
    monitor = ESP32Monitor(args.port, args.baud)
    monitor.start_monitoring(args.filter, args.save_log)

if __name__ == "__main__":
    main()