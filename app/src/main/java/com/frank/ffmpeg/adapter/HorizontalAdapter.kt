package com.frank.ffmpeg.adapter

import android.graphics.Color
import androidx.recyclerview.widget.RecyclerView

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button

import com.frank.ffmpeg.R
import com.frank.ffmpeg.listener.OnItemClickListener

/**
 * the horizontal adapter of RecyclerView
 * Created by frank on 2018/6/6.
 */

class HorizontalAdapter(private val itemList: List<String>?) : RecyclerView.Adapter<RecyclerView.ViewHolder>() {
    private var onItemClickListener: OnItemClickListener? = null
    private var lastClickPosition: Int = 0

    fun setOnItemClickListener(onItemClickListener: OnItemClickListener) {
        this.onItemClickListener = onItemClickListener
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        return OkViewHolder(LayoutInflater.from(parent.context).inflate(R.layout.item_select, parent, false))
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        val okViewHolder = holder as OkViewHolder
        okViewHolder.btnSelect.text = itemList!![position]
        okViewHolder.btnSelect.setTextColor(Color.DKGRAY)
        if (onItemClickListener != null) {
            okViewHolder.btnSelect.setOnClickListener {
                notifyItemChanged(lastClickPosition)
                //select the current color
                okViewHolder.btnSelect.setTextColor(Color.BLUE)
                onItemClickListener!!.onItemClick(okViewHolder.absoluteAdapterPosition)
                lastClickPosition = okViewHolder.absoluteAdapterPosition
            }
        }
    }

    override fun getItemCount(): Int {
        return itemList?.size ?: 0
    }

    private inner class OkViewHolder internal constructor(itemView: View) : RecyclerView.ViewHolder(itemView) {
        internal var btnSelect: Button = itemView.findViewById(R.id.btn_select)

    }

}
