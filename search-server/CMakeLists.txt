cmake_minimum_required(VERSION 3.1)	 # Проверка версии CMake.
										# Если версия установленой программы
										# старее указаной, произайдёт аварийный выход.

project(search_server)			# Название проекта

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(main ${SOURCES})	# Создает исполняемый файл с именем main
