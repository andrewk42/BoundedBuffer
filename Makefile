KIND:=NOBUSY                                    # KIND=BUSY->busy-wait (default), KIND=NOBUSY->no busy-wait
OPT:=

CXX = u++					                    # compiler
CXXFLAGS = -g -Wall -Wno-unused-label -MMD ${OPT} -D"${KIND}" -D"IMPLTYPE_${TYPE}"
ifeq (${MULTI}, ON)                             # MULTI=ON->use multiprocessors, default->uniprocessor with only user threads
CXXFLAGS += -multi                              # allow for multi flag in make
endif
ifeq (${OUT}, OFF)                              # OUT=OFF->Turn off Producer/Consumer print statements, default->on
CXXFLAGS += -D"NO_OUT"                           
endif
MAKEFILE_NAME = ${firstword ${MAKEFILE_LIST}}	# makefile name

OBJECTS = uc_boundedbuffer.o                    # object files forming 1st executable with prefix "q1"
EXEC = boundedBuffer

DEPENDS = ${OBJECTS:.o=.d}			            # substitute ".o" with ".d"

.PHONY : all clean

all : ${EXEC}					                # build executable

######################################################################################################

-include ImplKind

ifeq (${IMPLKIND},${KIND})			            # same implementation type as last time ?
${EXEC} : ${OBJECTS}
	${CXX} ${CXXFLAGS} $^ -o $@
else
ifeq (${KIND},)					                # no implementation type specified ?
# set type to previous type
KIND=${IMPLKIND}
${EXEC} : ${OBJECTS}
	${CXX} ${CXXFLAGS} $^ -o $@
else						                    # implementation type has changed
.PHONY : ${EXEC}
${EXEC} :
	rm -f ImplKind
	touch q1buffer.h
	${MAKE} ${EXEC} KIND=${KIND}
endif
endif

ImplKind :
	echo "IMPLKIND=${KIND}" > ImplKind

######################################################################################################

${OBJECTS} : ${MAKEFILE_NAME}			        # OPTIONAL : changes to this file => recompile

-include ${DEPENDS}

clean :						# remove files that can be regenerated
	rm -f *.d *.o ${EXEC} ImplKind
