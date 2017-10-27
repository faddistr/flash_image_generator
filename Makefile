CC=gcc

flash_img_gen:
	$(CC) -g $@.c -o $@

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core flash_img_gen