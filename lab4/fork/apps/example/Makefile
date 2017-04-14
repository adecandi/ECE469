default:
	cd app_fork; make

clean:
	cd app_fork; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -D m -u app_fork.dlx.obj 0; ee469_fixterminal
