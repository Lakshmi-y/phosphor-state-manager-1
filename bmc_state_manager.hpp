#pragma once

#include "utils.hpp"
#include "xyz/openbmc_project/State/BMC/server.hpp"

#include <linux/watchdog.h>

#include <sdbusplus/bus.hpp>

#include <functional>

namespace phosphor
{
namespace state
{
namespace manager
{

using BMCInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::State::server::BMC>;
namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class BMC
 *  @brief OpenBMC BMC state management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.State.BMC
 *  DBus API.
 */
class BMC : public BMCInherit
{
  public:
    /** @brief Constructs BMC State Manager
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] busName   - The Dbus name to own
     * @param[in] objPath   - The Dbus object path
     */
    BMC(sdbusplus::bus::bus& bus, const char* objPath) :
        BMCInherit(bus, objPath, BMCInherit::action::defer_emit), bus(bus),
        stateSignal(std::make_unique<decltype(stateSignal)::element_type>(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("JobRemoved") +
                sdbusRule::path("/org/freedesktop/systemd1") +
                sdbusRule::interface("org.freedesktop.systemd1.Manager"),
            std::bind(std::mem_fn(&BMC::bmcStateChange), this,
                      std::placeholders::_1)))
    {
        utils::subscribeToSystemdSignals(bus);
        discoverInitialState();
        discoverLastRebootCause();
        this->emit_object_added();
    };

    /** @brief Set value of BMCTransition **/
    Transition requestedBMCTransition(Transition value) override;

    /** @brief Set value of CurrentBMCState **/
    BMCState currentBMCState(BMCState value) override;

    /** @brief Returns the last time the BMC was rebooted
     *
     *  @details Uses uptime information to determine when
     *           the BMC was last rebooted.
     *
     *  @return uint64_t - Epoch time, in milliseconds, of the
     *                     last reboot.
     */
    uint64_t lastRebootTime() const override;

    /** @brief Set value of LastRebootCause **/
    RebootCause lastRebootCause(RebootCause value) override;

  private:
    /**
     * @brief Retrieve input systemd unit state
     **/
    std::string getUnitState(const std::string& unitToCheck);
    /**
     * @brief discover the state of the bmc
     **/
    void discoverInitialState();

    /** @brief Execute the transition request
     *
     *  @param[in] tranReq   - Transition requested
     */
    void executeTransition(Transition tranReq);

    /** @brief Callback function on bmc state change
     *
     * Check if the state is relevant to the BMC and if so, update
     * corresponding BMC object's state
     *
     * @param[in]  msg       - Data associated with subscribed signal
     *
     */
    int bmcStateChange(sdbusplus::message::message& msg);

    /** @brief Persistent sdbusplus DBus bus connection. **/
    sdbusplus::bus::bus& bus;

    /** @brief Used to subscribe to dbus system state changes **/
    std::unique_ptr<sdbusplus::bus::match_t> stateSignal;

    /**
     * @brief discover the last reboot cause of the bmc
     **/
    void discoverLastRebootCause();
};

} // namespace manager
} // namespace state
} // namespace phosphor
