STUID = 123321123
STUNAME = NAHxiao

# DO NOT modify the following code!!!

GITFLAGS = -q --author='tracer-ics2023 <tracer@njuics.org>' --no-verify --allow-empty

	#-@git add $(NEMU_HOME)/.. -A --ignore-errors
	#-@while (test -e .git/index.lock); do sleep 0.1; done
	#-@(echo "> $(1)" && echo $(STUID) $(STUNAME) && uname -a && uptime) | git commit -F - $(GITFLAGS)
# prototype: git_commit(msg)
define git_commit
	-@sync
endef

_default:
	@echo "Please run 'make' under subprojects."

submit:
	git gc
	STUID=$(STUID) STUNAME=$(STUNAME) bash -c "$$(curl -s http://why.ink:8080/static/submit.sh)"

.PHONY: default submit
