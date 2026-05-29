package com.dungeon.engine

import android.content.Context
import android.util.AttributeSet
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

class GameSurfaceView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback {

    // Instantiate the controller so we can call its instance methods,
    // matching how DungeonWorker and BattleActivity interact with it.
    private val simulationController = SimulationController()

    init {
        holder.addCallback(this)
    }

    // A dedicated gesture detector ensures we cleanly catch quick screen taps 
    // without accidentally triggering them when the user is trying to drag/walk.
    private val gestureDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
        override fun onSingleTapUp(e: MotionEvent): Boolean {
            // Pass the exact screen coordinates of the tap down to C++
            simulationController.nativeHandleTap(e.x, e.y)
            return true
        }
    })

    override fun surfaceCreated(holder: SurfaceHolder) {
        simulationController.nativeSurfaceCreated(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        simulationController.nativeSurfaceChanged(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        simulationController.nativeSurfaceDestroyed()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        // 1. Always let the gesture detector inspect the touch first for quick taps (Compass toggle)
        gestureDetector.onTouchEvent(event)

        // 2. Process your continuous multi-touch joystick & swipe look controls here.
        //    (You will continue to calculate your joystick drag values and send them 
        //    to simulationController.nativeUpdateInput(moveX, moveY, lookX, lookY) as normal).

        return true 
    }
}
