MK_PATH:=$(shell dirname $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
ACTONC=$(MK_PATH)/dist/bin/actonc --quiet
DDB_SERVER=../dist/bin/actondb
TESTS= \
	$(DDB_TESTS)
test:
	$(MAKE) $(TESTS)


ddb-tests:
	$(MAKE) $(DDB_TESTS)

DDB_TESTS=test_db_app test_db_app_no_quorum test_db_app_recovery test_db_app_resume_tcp_client test_db_app_resume_tcp_server
.PHONY: $(DDB_TESTS)

# Starts up a database cluster, checks membership is ok before proceeding to run
# a simple app. We do not really verify that the RTS uses the database - we
# assume it does and would fail catastrohpically if it encounters an error.
test_db_app:
	$(ACTONC) --root main test_db_app.act
	./test_db.py TestDbApps.test_app

test_db_app_no_quorum:
	@echo "Skipping because this is essentially broken"
	#$(ACTONC) --root main test_db_app.act
	#./test_db.py TestDbAppsNoQuorum

test_db_app_resume_tcp_server:
	$(ACTONC) --root main --dev rts/ddb_test_server.act
	./test_db.py TestDbApps.test_app_resume_tcp_server

test_db_app_resume_tcp_client:
	$(ACTONC) --root main --dev rts/ddb_test_server.act
	$(ACTONC) --root main --dev rts/ddb_test_client.act
	./test_db.py TestDbApps.test_app_resume_tcp_client


# Expect 9 threads given 7 workers + main process + IO
rts/wthreads1:
	$(ACTONC) --root main $@.act
	./$@ --rts-wthreads 7 & PID=$$! && ps -o thcount $${PID} | tail -n1 | awk '{ print $$1 }' | grep "^9$$"
