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

    private val controller = SimulationController()
    
    // Store finger IDs for true multi-touch
    private var movePointerId = -1
    private var lookPointerId = -1
    private var lastLookX = 0f
    private var lastLookY = 0f

    init {
        holder.addCallback(this)
    }

    private val gestureDetector = GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
        override fun onSingleTapUp(e: MotionEvent): Boolean {
            controller.nativeHandleTap(e.x, e.y)
            return true
        }
    })

    override fun surfaceCreated(holder: SurfaceHolder) { controller.nativeSurfaceCreated(holder.surface) }
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) { controller.nativeSurfaceChanged(width, height) }
    override fun surfaceDestroyed(holder: SurfaceHolder) { controller.nativeSurfaceDestroyed() }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        // 1. Always pass to gesture detector first to catch compass/menu taps
        gestureDetector.onTouchEvent(event)

        val action = event.actionMasked
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)

        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                // Assign first finger to movement, second to looking
                if (movePointerId == -1) {
                    movePointerId = pointerId
                } else if (lookPointerId == -1) {
                    lookPointerId = pointerId
                    lastLookX = event.getX(pointerIndex)
                    lastLookY = event.getY(pointerIndex)
                }
            }
            MotionEvent.ACTION_MOVE -> {
                // Process Look (Delta tracking)
                if (lookPointerId != -1) {
                    val idx = event.findPointerIndex(lookPointerId)
                    val curX = event.getX(idx)
                    val curY = event.getY(idx)
                    controller.nativeUpdateInput(0f, 0f, curX - lastLookX, curY - lastLookY)
                    lastLookX = curX
                    lastLookY = curY
                }
                
                // Process Move (Simple relative joystick simulation)
                if (movePointerId != -1) {
                    val idx = event.findPointerIndex(movePointerId)
                    // Simplified: map screen position to movement axes
                    controller.nativeUpdateInput(event.getX(idx) / width * 2 - 1, event.getY(idx) / height * 2 - 1, 0f, 0f)
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                if (pointerId == movePointerId) movePointerId = -1
                if (pointerId == lookPointerId) lookPointerId = -1
            }
        }
        return true
    }
}
