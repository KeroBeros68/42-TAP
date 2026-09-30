
GUI	:= 42TAP

install: $GUI

$GUI:
	make -C clients/gui
	cp clients/gui/$(GUI) ./$(GUI)

fclean:
	make -C clients/gui fclean
	rm $(GUI)

clean:
	make -C clients/gui clean
