import { all } from 'redux-saga/effects'
import { notificationSaga } from './Notification.saga'
import { authSaga } from './Authentication.saga'
import { deviceSaga } from './Device.saga'
import { pairingSaga } from './Pairing.saga'
import { deviceHistorySaga } from './DeviceHistory.saga'
import { deviceRemovalSaga } from './DeviceRemoval.saga'
import { commandsSaga } from './Command.saga'

export function* RootSaga() {
	yield all([
		notificationSaga(),
		authSaga(),
		deviceSaga(),
		pairingSaga(),
		deviceHistorySaga(),
		deviceRemovalSaga(),
		commandsSaga(),
	])
}
