SET(SrlSrlSRC
        config/SrlSrlConfig.h
        model/SrlSrlModel.h
        model/SrlSrlModel.cpp)

add_executable(Srltrain train.cpp process/TrainSrlSrl.cpp ${SrlSrlSRC})
target_link_libraries(Srltrain dynet ${LIBS})

add_executable(Srlpred pred.cpp process/PredSrlSrl.cpp ${SrlSrlSRC})
target_link_libraries(Srlpred dynet ${LIBS})

add_library(srl_srl_static_lib STATIC ${SrlSrlSRC})
target_link_libraries(srl_srl_static_lib dynet ${LIBS})
