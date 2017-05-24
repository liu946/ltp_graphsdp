SET(SrlPiSRC
        config/SrlPiConfig.h
        model/SrlPiModel.h
        )

add_executable(Pitrain train.cpp process/TrainSrlPi.cpp ${SrlPiSRC})
target_link_libraries(Pitrain dynet ${LIBS})

add_executable(Pipred pred.cpp process/PredSrlPi.cpp ${SrlPiSRC})
target_link_libraries(Pipred dynet ${LIBS})
