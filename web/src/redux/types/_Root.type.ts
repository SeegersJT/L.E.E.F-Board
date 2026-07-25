import type { AuthState } from './Authentication.type'
import type { NotificationState } from './Notification.type'
import type { DeviceState } from './Device.type'
import type { PairingState } from './Pairing.type'
import type { DeviceHistoryState } from './DeviceHistory.type'
import type { DeviceRemovalState } from './DeviceRemoval.type'
import type { CommandsState } from './Command.type'

export interface RootState {
	system: {
		notification: NotificationState
	}
	auth: AuthState
	devices: DeviceState
	pairing: PairingState
	deviceHistory: DeviceHistoryState
	deviceRemoval: DeviceRemovalState
	commands: CommandsState
}
