default:
	cd makeprocs; make
	cd hello_world; make
	cd part1; make
	cd part2; make
	cd part3; make
	cd part4; make
	cd part5; make
	cd part6; make

clean:
	cd makeprocs; make clean
	cd hello_world; make clean
	cd part1; make clean
	cd part2; make clean
	cd part3; make clean
	cd part4; make clean
	cd part5; make clean
	cd part6; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 5; ee469_fixterminal
