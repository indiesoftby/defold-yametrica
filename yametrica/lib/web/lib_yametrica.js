var LibraryYaMetrica = {
    $YaMetrica: {
        parseJson: function (json) {
            try {
                return JSON.parse(json);
            } catch (e) {
                console.warn(e);
                return null;
            }
        },
    },

    YaMetrica_Hit: function (counterId, url, cOptions, cOptionsLen) {
        // console.log("YaMetrica_Hit", counterId, UTF8ToString(url), cOptions === 0 ? "NULL" : UTF8ToString(cOptions, cOptionsLen));

        try {
            var options = cOptions === 0 ? {} : YaMetrica.parseJson(UTF8ToString(cOptions, cOptionsLen));
            ym(counterId, "hit", UTF8ToString(url), options);
        } catch (e) {
            console.warn(e);
        }
    },

    YaMetrica_NotBounce: function (counterId, cOptions, cOptionsLen) {
        // console.log("YaMetrica_NotBounce", counterId, cOptions === 0 ? "NULL" : UTF8ToString(cOptions, cOptionsLen));

        try {
            var options = cOptions === 0 ? {} : YaMetrica.parseJson(UTF8ToString(cOptions, cOptionsLen));
            ym(counterId, "notBounce", options);
        } catch (e) {
            console.warn(e);
        }
    },

    YaMetrica_Params: function (counterId, cVisitParams, cVisitParamsLen, cGoalParams, cGoalParamsLen) {
        // console.log("YaMetrica_Params", counterId, cVisitParams === 0 ? "NULL" : UTF8ToString(cVisitParams, cVisitParamsLen), cGoalParams === 0 ? "NULL" : UTF8ToString(cGoalParams, cGoalParamsLen));

        try {
            var visitParams = YaMetrica.parseJson(UTF8ToString(cVisitParams, cVisitParamsLen));
            if (cGoalParams === 0) {
                ym(counterId, "params", visitParams);
            } else {
                var goalParams = YaMetrica.parseJson(UTF8ToString(cGoalParams, cGoalParamsLen));
                ym(counterId, "params", visitParams, goalParams);
            }
        } catch (e) {
            console.warn(e);
        }
    },

    YaMetrica_ReachGoal: function (counterId, target, cParams, cParamsLen) {
        // console.log("YaMetrica_ReachGoal", counterId, UTF8ToString(target), cParams === 0 ? "NULL" : UTF8ToString(cParams, cParamsLen));

        try {
            var params = cParams === 0 ? {} : YaMetrica.parseJson(UTF8ToString(cParams, cParamsLen));
            ym(counterId, "reachGoal", UTF8ToString(target), params);
        } catch (e) {
            console.warn(e);
        }
    },

    YaMetrica_UserParams: function (counterId, cParams, cParamsLen) {
        // console.log("YaMetrica_UserParams", counterId, cParams === 0 ? "NULL" : UTF8ToString(cParams, cParamsLen));

        try {
            var params = cParams === 0 ? {} : YaMetrica.parseJson(UTF8ToString(cParams, cParamsLen));
            ym(counterId, "userParams", params);
        } catch (e) {
            console.warn(e);
        }
    },
};

autoAddDeps(LibraryYaMetrica, "$YaMetrica");
mergeInto(LibraryManager.library, LibraryYaMetrica);
