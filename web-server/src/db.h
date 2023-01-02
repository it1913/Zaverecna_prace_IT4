typedef String ExerciseId;
typedef int SessionId;
typedef String ParticipantId;
typedef uint32_t ExerciseTime;

class Record {
    private:
        ExerciseId _exerciseId;
        SessionId _sessionId;
        ParticipantId _participantId;
        int _step;
        ExerciseTime _time;
        int _state;
    public:
        Record(ExerciseId exerciseId, SessionId sessionId, ParticipantId participantId) {
            _exerciseId = exerciseId;
            _sessionId = sessionId;
            _participantId = participantId;
        }

        ~Record() {
        }

        void init(int step, ExerciseTime time, int state) {
            _step = step;
            _time = time;
            _state = state;
        }

        String data() {
            return
            "exerciseId="+_exerciseId+
            "&sessionId="+_sessionId+
            "&participantId="+_participantId+
            "&step="+String(_step)+
            "&time="+stopwatch(_time)+
            "&state="+String(_state);
        }
};