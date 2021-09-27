package com.frank.ffmpeg.adapter

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.frank.ffmpeg.R
import com.frank.ffmpeg.listener.OnItemClickListener
import java.util.*

/**
 * waterfall layout adapter
 * Created by frank on 2021/9/27.
 */
class WaterfallAdapter() : RecyclerView.Adapter<RecyclerView.ViewHolder>() {

    companion object {
        const val DEFAULT_MARGIN = 5
    }

    private var itemList: List<String>? = null

    private var heightList: ArrayList<Int>? = null

    private var onItemClickListener: OnItemClickListener? = null

    constructor(itemList: List<String>?) : this() {
        this.itemList = itemList

        heightList = arrayListOf(itemList!!.size)
        for (i in itemList.indices) {
            val height = Random().nextInt(20) + DEFAULT_MARGIN
            heightList?.add(height)
        }
    }

    fun setOnItemClickListener(onItemClickListener: OnItemClickListener) {
        this.onItemClickListener = onItemClickListener
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        return RandomHolder(LayoutInflater.from(parent.context).inflate(R.layout.item_waterfall ,parent, false))
    }

    override fun getItemCount(): Int {
        return itemList?.size ?: 0
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        val randomHolder = holder as RandomHolder
        val layoutParams:ViewGroup.MarginLayoutParams =
                randomHolder.txtComponent.layoutParams as ViewGroup.MarginLayoutParams
        var margin = DEFAULT_MARGIN
        if (heightList!![position] > DEFAULT_MARGIN) margin = heightList!![position]
        layoutParams.topMargin = margin
        layoutParams.bottomMargin = margin
        randomHolder.txtComponent.layoutParams = layoutParams

        randomHolder.txtComponent.text = itemList!![position]
        if (onItemClickListener != null) {
            randomHolder.txtComponent.setOnClickListener {
                onItemClickListener!!.onItemClick(randomHolder.absoluteAdapterPosition)
            }
        }
    }

    private inner class RandomHolder internal constructor(itemView: View) : RecyclerView.ViewHolder(itemView) {
        internal val txtComponent :TextView = itemView.findViewById(R.id.txt_component)
    }

}