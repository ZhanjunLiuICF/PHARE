
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>

#include "phare/include.h"

namespace py = pybind11;

namespace PHARE
{
class StreamAppender : public SAMRAI::tbox::Logger::Appender
{
public:
    StreamAppender(std::ostream* stream) { d_stream = stream; }
    void logMessage(const std::string& message, const std::string& filename, const int line)
    {
        (*d_stream) << "At :" << filename << " line :" << line << " message: " << message
                    << std::endl;
    }

private:
    std::ostream* d_stream;
};

class SamraiLifeCycle
{
public:
    static SamraiLifeCycle& INSTANCE()
    {
        static SamraiLifeCycle i;
        return i;
    }

    SamraiLifeCycle()
    {
        SAMRAI::tbox::SAMRAI_MPI::init(0, nullptr);
        SAMRAI::tbox::SAMRAIManager::initialize();
        SAMRAI::tbox::SAMRAIManager::startup();

        std::shared_ptr<SAMRAI::tbox::Logger::Appender> appender
            = std::make_shared<StreamAppender>(StreamAppender{&std::cout});
        SAMRAI::tbox::Logger::getInstance()->setWarningAppender(appender);
    }
    ~SamraiLifeCycle()
    {
        SAMRAI::tbox::SAMRAIManager::shutdown();
        SAMRAI::tbox::SAMRAIManager::finalize();
        SAMRAI::tbox::SAMRAI_MPI::finalize();
    }

    // the simulator must be destructed before the
    //  variable database is reset, or segfault.
    void reset()
    {
        PHARE::initializer::PHAREDictHandler::INSTANCE().stop();
        SAMRAI::tbox::SAMRAIManager::shutdown();
        SAMRAI::tbox::SAMRAIManager::startup();
    }

    // we must control the order of destruction as python gives no guarantees on GC order.
    std::shared_ptr<ISimulator> getISimulator()
    {
        auto sim = PHARE::getSimulator();
        std::shared_ptr<ISimulator> ptr{sim.get(), [](ISimulator*) {
                                            /* no delete on ptr destruct */
                                        }};
        sim.release();
        return ptr;
    }

    std::shared_ptr<diagnostic::IDiagnosticsManager>
    getIDiagnosticsManager(std::shared_ptr<ISimulator> const& sim)
    {
        if (!sim)
            throw std::runtime_error("getDiagnosticsManager requires simulator be created first");
        auto rdi = std::make_unique<RuntimeDiagnosticInterface>(*sim);
        std::shared_ptr<diagnostic::IDiagnosticsManager> ptr{rdi->dMan.get(),
                                                             [](diagnostic::IDiagnosticsManager*) {
                                                                 /* no delete on ptr destruct */
                                                             }};
        rdi.release();
        return ptr;
    }
};

PYBIND11_MODULE(test_simulator, m)
{
    SamraiLifeCycle::INSTANCE(); // init
    py::class_<ISimulator, std::shared_ptr<ISimulator>>(m, "ISimulator")
        .def("initialize", &PHARE::ISimulator::initialize)
        .def("advance", &PHARE::ISimulator::advance)
        .def("startTime", &PHARE::ISimulator::startTime)
        .def("endTime", &PHARE::ISimulator::endTime)
        .def("timeStep", &PHARE::ISimulator::timeStep)
        .def("to_str", &PHARE::ISimulator::to_str);

    py::class_<diagnostic::IDiagnosticsManager, std::shared_ptr<diagnostic::IDiagnosticsManager>>(
        m, "IDiagnosticsManager")
        .def("dump", &PHARE::diagnostic::IDiagnosticsManager::dump);

    m.def("make_simulator", []() { return SamraiLifeCycle::INSTANCE().getISimulator(); });
    m.def("make_diagnostic_manager", [](std::shared_ptr<ISimulator> const& sim) {
        return SamraiLifeCycle::INSTANCE().getIDiagnosticsManager(sim);
    });

    m.def("unmake", [](std::shared_ptr<ISimulator>& sim) { sim.reset(); });
    m.def("unmake", [](std::shared_ptr<diagnostic::IDiagnosticsManager>& dman) { dman.reset(); });

    m.def("reset", []() {
        py::gil_scoped_release release;
        SamraiLifeCycle::INSTANCE().reset();
    });
}
} // namespace PHARE
