import { combineReducers } from 'redux'
import { notificationReducer } from './Notification.reducer'
import { AuthReducer } from './Authentication.reducer'
import { DeviceReducer } from './Device.reducer'
import { PairingReducer } from './Pairing.reducer'
import { DeviceHistoryReducer } from './DeviceHistory.reducer'
import { DeviceRemovalReducer } from './DeviceRemoval.reducer'
import { CommandsReducer } from './Command.reducer'

export const RootReducer = combineReducers({
	system: combineReducers({
		notification: notificationReducer,
	}),
	auth: AuthReducer,
	devices: DeviceReducer,
	pairing: PairingReducer,
	deviceHistory: DeviceHistoryReducer,
	deviceRemoval: DeviceRemovalReducer,
	commands: CommandsReducer,
})
