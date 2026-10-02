
GUI	:= 42TAP

install: $GUI

run: $GUI
	./$(GUI)

$GUI:
	make -C clients/gui
	mv clients/gui/$(GUI) ./$(GUI)

fclean:
	make -C clients/gui fclean
	rm $(GUI)

clean:
	make -C clients/gui clean
