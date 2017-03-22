default:
	cd makeprocs; make
	cd inj_co; make
	cd inj_s2; make
	cd make_so4; make
	cd sp_co; make
	cd sp_s2; make

clean:
	cd makeprocs; make clean
	cd inj_co; make clean
	cd inj_s2; make clean
	cd make_so4; make clean
	cd sp_co; make clean
	cd sp_s2; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 1 4; ee469_fixterminal
